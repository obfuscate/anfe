#include <engine/services/imgui_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/input_service.h>
#include <engine/services/render_service.h>
#include <engine/services/windows_service.h>

#include <engine/integration/imgui/imgui_impl_win32.h>
#include <engine/integration/imgui/imgui_impl_dx12.h>

#include <LLGL/Backend/Direct3D12/NativeHandle.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	reflection::Service<ImGUIService>("ImGUIService");
}


//-- Helper class to allocate D3D12 descriptors
class D3D12DescriptorHeapAllocator
{
	ID3D12DescriptorHeap* d3dHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE d3dCPUHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE d3dGPUHandle;
	UINT d3dHandleSize = 0;
	std::vector<UINT> freeIndices;

public:
	D3D12DescriptorHeapAllocator(ID3D12Device* d3dDevice, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptors)
	{
		// Create D3D12 descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC d3dSRVDescriptorHeapDesc = {};
		{
			d3dSRVDescriptorHeapDesc.Type = type;
			d3dSRVDescriptorHeapDesc.NumDescriptors = numDescriptors;
			d3dSRVDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			d3dSRVDescriptorHeapDesc.NodeMask = 0;
		}
		HRESULT result = d3dDevice->CreateDescriptorHeap(&d3dSRVDescriptorHeapDesc, IID_PPV_ARGS(&d3dHeap));
		LLGL_VERIFY(SUCCEEDED(result));

		d3dCPUHandle = d3dHeap->GetCPUDescriptorHandleForHeapStart();
		d3dGPUHandle = d3dHeap->GetGPUDescriptorHandleForHeapStart();
		d3dHandleSize = d3dDevice->GetDescriptorHandleIncrementSize(type);

		//-- Initialize free indices
		freeIndices.reserve(numDescriptors);
		for (UINT n = numDescriptors; n > 0; --n)
		{
			freeIndices.push_back(n - 1);
		}
	}

	~D3D12DescriptorHeapAllocator()
	{
		safeRelease(d3dHeap);
	}

	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE& outCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGPUHandle)
	{
		LLGL_VERIFY(!freeIndices.empty());

		UINT index = freeIndices.back();
		freeIndices.pop_back();

		outCPUHandle.ptr = d3dCPUHandle.ptr + (index * d3dHandleSize);
		outGPUHandle.ptr = d3dGPUHandle.ptr + (index * d3dHandleSize);
	}

	void Free(D3D12_CPU_DESCRIPTOR_HANDLE inCPUHandle, D3D12_GPU_DESCRIPTOR_HANDLE inGPUHandle)
	{
		const UINT cpuIndex = static_cast<UINT>((inCPUHandle.ptr - d3dCPUHandle.ptr) / d3dHandleSize);
		const UINT gpuIndex = static_cast<UINT>((inGPUHandle.ptr - d3dGPUHandle.ptr) / d3dHandleSize);
		LLGL_VERIFY(cpuIndex == gpuIndex);
		freeIndices.push_back(cpuIndex);
	}

	ID3D12DescriptorHeap* GetNative() const
	{
		return d3dHeap;
	}
};


class D3D12Backend : public ImGUIService::Backend
{
public:
	D3D12Backend(LLGL::RenderSystem* renderer)
	{
		//-- Create SRV descriptor heap for ImGui's internal resources
		LLGL::Direct3D12::RenderSystemNativeHandle nativeDeviceHandle;
		renderer->GetNativeHandle(&nativeDeviceHandle, sizeof(nativeDeviceHandle));
		m_device = nativeDeviceHandle.device;
		m_commandQueue = nativeDeviceHandle.commandQueue;

		m_heapAllocator = DescriptorHeapAllocatorPtr(new D3D12DescriptorHeapAllocator{ m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64 });

		createResources();
	}

	~D3D12Backend()
	{
		m_heapAllocator.reset();
		safeRelease(m_commandQueue);
		safeRelease(m_device);
	}

	void initializeContext(WindowContext& ctx) override
	{
		Backend::initializeContext(ctx);

		auto& ws = service<WindowsService>();
		//-- Initialize ImGui D3D12 backend
		ImGui_ImplDX12_InitInfo imGuiInfo = {};
		{
			imGuiInfo.Device = m_device;
			imGuiInfo.CommandQueue = m_commandQueue;
			imGuiInfo.NumFramesInFlight = ctx.swapChain->GetNumSwapBuffers();
			imGuiInfo.RTVFormat = LLGL::DXTypes::ToDXGIFormat(ws.mainWindow()->swapChain->GetColorFormat());
			imGuiInfo.DSVFormat = LLGL::DXTypes::ToDXGIFormat(ws.mainWindow()->swapChain->GetDepthStencilFormat());
			imGuiInfo.UserData = static_cast<void*>(m_heapAllocator.get());
			imGuiInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* outCPUDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE* outGPUDescHandle)
				{
					auto heapAllocator = static_cast<D3D12DescriptorHeapAllocator*>(info->UserData);
					heapAllocator->Alloc(*outCPUDescHandle, *outGPUDescHandle);
				};
			imGuiInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE inCPUDescHandle, D3D12_GPU_DESCRIPTOR_HANDLE inGPUDescHandle)
				{
					auto heapAllocator = static_cast<D3D12DescriptorHeapAllocator*>(info->UserData);
					heapAllocator->Free(inCPUDescHandle, inGPUDescHandle);
				};
		}
		ImGui_ImplDX12_Init(&imGuiInfo);
	}

	void releaseContext(WindowContext& ctx) override
	{
		ImGui::SetCurrentContext(ctx.imGuiContext);

		ImGui_ImplDX12_Shutdown();

		Backend::releaseContext(ctx);
	}

	void endFrame(LLGL::CommandBuffer* commandList) override
	{
		LLGL::Direct3D12::CommandBufferNativeHandle nativeContextHandle;
		commandList->GetNativeHandle(&nativeContextHandle, sizeof(nativeContextHandle));
		auto d3dCommandList = nativeContextHandle.commandList;

		ID3D12DescriptorHeap* d3dHeap = m_heapAllocator->GetNative();
		d3dCommandList->SetDescriptorHeaps(1, &d3dHeap);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3dCommandList);
	}
private:
	using DescriptorHeapAllocatorPtr = std::unique_ptr<D3D12DescriptorHeapAllocator>;
	DescriptorHeapAllocatorPtr m_heapAllocator;
	ID3D12Device* m_device = nullptr;
	ID3D12CommandQueue* m_commandQueue = nullptr;
};


ImGuiContext* createImGuiContext()
{
	ImGuiContext* imGuiContext = ImGui::CreateContext();
	{
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	}
	return imGuiContext;
}


void forwardInputToImGui(ImGUIService::Backend::WindowContext& context)
{
	//-- Forward user input to ImGui
	ImGuiIO& io = ImGui::GetIO();

	io.AddMousePosEvent(static_cast<float>(context.mousePosInWindow.x), static_cast<float>(context.mousePosInWindow.y));

	auto& is = service<InputService>();
	if (is.keyDown(LLGL::Key::LButton))
	{
		io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, true);
	}
	if (is.keyUp(LLGL::Key::LButton))
	{
		io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
		io.AddMouseButtonEvent(ImGuiMouseButton_Left, false);
	}
}

} //-- unnamed.


void ImGUIService::Backend::initialize()
{
	for (auto& ctx : m_windowContexts)
	{
		initializeContext(ctx);
	}
}


void ImGUIService::Backend::release()
{
	for (auto& ctx : m_windowContexts)
	{
		releaseContext(ctx);
	}
}


void ImGUIService::Backend::beginFrame(LLGL::CommandBuffer* commandList)
{
	auto ctx = m_windowContexts.front();
	//-- ToDo: multi-window.
	ImGui::SetCurrentContext(ctx.imGuiContext);
	
	forwardInputToImGui(ctx);
}


void ImGUIService::Backend::createResources()
{
	//-- Create new swap-chain/ImGui context connection
	WindowContext context;
	{
		context.swapChain = service<WindowsService>().mainWindow()->swapChain;
		context.imGuiContext = createImGuiContext();
	}
	m_windowContexts.push_back(context);
	/*std::shared_ptr<WindowEventListener> eventListener = std::make_shared<WindowEventListener>(this);

	auto AddWindowWithSwapChain = [this, &eventListener](int x, int y, unsigned width, unsigned height) -> void
		{
			//-- Create swap chain
			LLGL::SwapChainDescriptor swapChainDesc;
			{
				swapChainDesc.resolution = { width, height };
				swapChainDesc.resizable = true;
			}
			LLGL::SwapChain* swapChain = renderer->CreateSwapChain(swapChainDesc);

			if (scene.showcase.isVsync)
				swapChain->SetVsyncInterval(1);

			//-- Register callback to update swap-chain on window resize
			LLGL::Window& window = LLGL::CastTo<LLGL::Window>(swapChain->GetSurface());

			window.AddEventListener(eventListener);
			window.SetPosition(LLGL::Offset2D{ x, y });

			// Initialize view settings
			g_swapChains.push_back(swapChain);

			// Create new swap-chain/ImGui context connection
			WindowContext context;
			{
				context.swapChain = swapChain;
				context.imGuiContext = NewImGuiContext();
			}
			m_windowContexts.push_back(context);
		};

	LLGL::Display* display = LLGL::Display::GetPrimary();
	LLGL_VERIFY(display != nullptr);

	const LLGL::Extent2D displaySize = display->GetDisplayMode().resolution;

	constexpr unsigned resX = 600;
	constexpr unsigned resY = 800;
	constexpr unsigned windowMargin = 20;*/

	//AddWindowWithSwapChain(static_cast<int>(displaySize.width / 2 - resX - windowMargin), static_cast<int>(displaySize.height / 2 - resY / 2), resX, resY);
	//AddWindowWithSwapChain(static_cast<int>(displaySize.width / 2 + windowMargin), static_cast<int>(displaySize.height / 2 - resY / 2), resX, resY);

	// Create command buffer with immediate context
	//cmdBuffer = renderer->CreateCommandBuffer(LLGL::CommandBufferFlags::ImmediateSubmit);*/
}


void ImGUIService::Backend::initializeContext(WindowContext& ctx)
{
	ImGui::SetCurrentContext(ctx.imGuiContext);

	//-- Setup Dear ImGui style
	ImGui::StyleColorsDark();

	//-- Initialize current ImGui context
	//-- ToDo: Make platform independent.
	{
		LLGL::NativeHandle nativeHandle;
		ctx.swapChain->GetSurface().GetNativeHandle(&nativeHandle, sizeof(nativeHandle));

		ImGui_ImplWin32_Init(nativeHandle.window);
	}

	//-- Connect swap-chain and ImGui context with window
	LLGL::CastTo<LLGL::Window>(ctx.swapChain->GetSurface()).SetUserData(&ctx);

	//lastTick = LLGL::Timer::Tick();
}


void ImGUIService::Backend::releaseContext(WindowContext& ctx)
{
	ImGui::SetCurrentContext(ctx.imGuiContext);

	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext(ctx.imGuiContext);
}


bool ImGUIService::initialize()
{
	IMGUI_CHECKVERSION();

	auto& rs = service<RenderService>();
	switch (rs.gapi())
	{
	case GraphicsAPI::DirectX12:
	{
		m_backend = std::make_unique<D3D12Backend>(rs.renderer());
		break;
	}
	default:
	{
		ENGINE_FAIL("[ImGUI]: Can't initialize ImGUI using Unknown Graphics API");
		return false;
	}
	}

	m_backend->initialize();
	m_commandList = rs.commandListPool().imguiCommandList();

	//-- Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());

	return true;
}


void ImGUIService::release()
{
	m_backend->release();
	m_backend.reset();
}


void ImGUIService::tick()
{
	auto& swapChain = *service<WindowsService>().mainWindow()->swapChain;
	m_commandList->Begin();
	m_commandList->BeginRenderPass(swapChain);
	m_commandList->SetViewport(swapChain.GetResolution());
	m_commandList->PushDebugGroup("RenderGUI");

	m_backend->beginFrame(m_commandList);
	//-- ToDO: Reconsider later.
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX12_NewFrame();
	ImGui::NewFrame();
}


void ImGUIService::postTick()
{
	ImGui::Render();
	m_backend->endFrame(m_commandList);

	m_commandList->PopDebugGroup();
	m_commandList->EndRenderPass();
	m_commandList->End();
}

} //-- engine.
