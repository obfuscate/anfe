#include <engine/render/d3d12/backend.h>
#include <engine/assert.h>
#include <engine/helpers.h>
#include <engine/math.h>

using Microsoft::WRL::ComPtr;
using namespace std::string_view_literals;

namespace engine::render::d3d12
{


namespace
{

inline D3D_FEATURE_LEVEL createFakeDevice(IDXGIAdapter1* adapter, const std::vector<D3D_FEATURE_LEVEL>& desiredFeatureLevels)
{
	auto result = D3D_FEATURE_LEVEL_1_0_CORE;

	DXGI_ADAPTER_DESC1 desc;
	adapter->GetDesc1(&desc);

	if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
	{
		//-- Don't select the Basic Render Driver adapter.
		return result;
	}

	//-- Check to see whether the adapter supports Direct3D 12, but don't create the actual device yet.
	for (auto fl : desiredFeatureLevels)
	{
		if (SUCCEEDED(D3D12CreateDevice(adapter, fl, _uuidof(ID3D12Device), nullptr)))
		{
			result = fl;
			break;
		}
	}

	return result;
}


//-- Also returns max possible FL.
D3D_FEATURE_LEVEL getHardwareAdapter(
	IDXGIFactory7* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter,
	const std::vector<D3D_FEATURE_LEVEL>& desiredFeatureLevels)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory7> factory;

	D3D_FEATURE_LEVEL maxFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory))))
	{
		const auto preference = requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED;
		UINT adapterIndex = 0;
		while (SUCCEEDED(factory->EnumAdapterByGpuPreference(adapterIndex, preference, IID_PPV_ARGS(&adapter))))
		{
			maxFeatureLevel = createFakeDevice(adapter.Get(), desiredFeatureLevels);

			if (maxFeatureLevel != D3D_FEATURE_LEVEL_1_0_CORE)
			{
				break;
			}

			++adapterIndex;
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			maxFeatureLevel = createFakeDevice(adapter.Get(), desiredFeatureLevels);
		}
	}

	*ppAdapter = adapter.Detach();

	return maxFeatureLevel;
}

struct Vertex
{
	engine::math::vec3 position;
	uint32_t color;
};

inline UINT calculateConstantBufferByteSize(UINT byteSize)
{
	// Constant buffer size is required to be aligned.
	return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

} //-- unnamed.


bool Backend::initialize(const Desc& desc)
{
	m_shaderCompiler.initialize();

	uint32_t dxgiFactoryFlags = 0;

	//-- Enable the debug layer (requires the Graphics Tools "optional feature").
	//-- NOTE: Enabling the debug layer after device creation will invalidate the active device.
	if (hasFlag(desc.flags, Flags::DebugLayer))
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			//-- Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}

	ComPtr<IDXGIFactory7> factory;
	HRESULT ok = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a DXGI Factory.");

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	std::vector<D3D_FEATURE_LEVEL> requestedFeatureLevels = { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
	auto featureLevel = getHardwareAdapter(factory.Get(), &hardwareAdapter, true, requestedFeatureLevels);

	ok = D3D12CreateDevice(hardwareAdapter.Get(), featureLevel, IID_PPV_ARGS(&m_device));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a D3D12 device.");

	//-- Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ok = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_graphicsCommandQueue));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a direct command queue.");

	//-- Viewport.
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(desc.width), static_cast<float>(desc.height), 0.0f, 1.0f);
	m_scissorRect = CD3DX12_RECT(0, 0, desc.width, desc.height);

	//-- Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = desc.numBuffers;
	swapChainDesc.Width = desc.width;
	swapChainDesc.Height = desc.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //-- ToDo: Use sRGB?
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	HWND wnd = static_cast<HWND>(desc.hwnd);
	ComPtr<IDXGISwapChain1> swapChain;
	ok = factory->CreateSwapChainForHwnd(m_graphicsCommandQueue.Get(), wnd, &swapChainDesc,
		nullptr, nullptr, &swapChain);
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a swap chain for HWND.");

	//-- does not support fullscreen transitions. ToDo: Reconsider later.
	ok = factory->MakeWindowAssociation(wnd, DXGI_MWA_NO_ALT_ENTER);
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't make window association.");

	ok = swapChain.As(&m_swapChain);
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't assign a swap chain.");

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//-- Create descriptor heaps.
	{
		//-- Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = desc.numBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ok = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a descriptor heap for the backbuffers.");

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//-- Describe and create a constant buffer view (CBV) descriptor heap.
		//-- Flags indicate that this descriptor heap can be bound to the pipeline 
		//-- and that descriptors contained in it can be referenced by a root table.
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 1;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ok = m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a descriptor heap for constant buffers.");
	}

	//-- Create frame resources (a RTV for each frame).
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		m_renderTargets.resize(desc.numBuffers);
		for (UINT n = 0; n < desc.numBuffers; n++)
		{
			ok = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			ENGINE_ASSERT(SUCCEEDED(ok));

			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ok = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a command allocator.");

	ok = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&m_bundleAllocator));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a bundle command allocator.");

	//-- Create the command list.
	ok = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a command list.");

	//-- Command lists are created in the recording state, but there is nothing to record yet.
	//-- The main loop expects it to be closed, so close it now.
	ok = m_commandList->Close();
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't close a command list.");

	//-- Create synchronization objects.
	{
		ok = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a fence.");
		m_fenceValue = 1;

		//-- Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ok = HRESULT_FROM_WIN32(GetLastError());
			ENGINE_ASSERT(SUCCEEDED(ok));
		}
	}

	//-- Create a root signature.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
		CD3DX12_ROOT_PARAMETER1 rootParameters[1];

		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[0].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		//-- Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		//-- The serialized version could be stored in a file on disk for quick loading, eliminating the need to recreate it each time.
		//-- ToDo: Root signatures can also be defined directly within shader code.
		//-- In such cases, the shader code and the root signature are compiled together into the same memory blob.
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			//-- This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

			if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
			{
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			ok = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
			ENGINE_ASSERT(SUCCEEDED(ok));
		}

		ok = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
		ENGINE_ASSERT(SUCCEEDED(ok));
	}

	//-- Create the pipeline state, which includes compiling and loading shaders.
	{
		auto shader = m_shaderCompiler.compile("/shaders/test_shader.hlsl"sv);
		if (!shader->ready())
		{
			ENGINE_FAIL("Can't load the test shader");
		}

		//-- Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		auto vertexShader = shader->shader(resources::ShaderResource::Type::Vertex);
		auto pixelShader = shader->shader(resources::ShaderResource::Type::Pixel);

		//-- Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.first, vertexShader.second);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.first, pixelShader.second);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ok = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a PSO.");

		shader->release(); //-- release IDxcBlob memory. Todo: Reconsider later.
	}

	//-- Create the vertex buffer.
	{
		const float aspectRatio = desc.width / static_cast<float>(desc.height);
		//-- Define the geometry for a triangle.
		Vertex triangleVertices[] =
		{
			{ math::vec3(0.0f, 0.25f * aspectRatio, 0.0f), math::color(1.0f, 0.0f, 0.0f, 1.0f).BGRA()},
			{ math::vec3(0.25f, -0.25f * aspectRatio, 0.0f), math::color(0.0f, 1.0f, 0.0f, 1.0f).BGRA() },
			{ math::vec3(-0.25f, -0.25f * aspectRatio, 0.0f), math::color(0.0f, 0.0f, 1.0f, 1.0f).BGRA() }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		//-- ToDo: Reconsider later.
		//-- Note: using upload heaps to transfer static data like vert buffers is not recommended.
		//-- Every time the GPU needs it, the upload heap will be marshalled over.
		//-- Please read up on Default Heap usage. An upload heap is used here for code simplicity
		//-- and because there are very few verts to actually transfer.
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);
		ok = m_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_vertexBuffer));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a vertex buffer.");

		//-- Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);        //-- We do not intend to read from this resource on the CPU.
		//-- ToDo: It returns (as an output parameter) a pointer to the CPU-visible GPU heap memory where the resource is stored
		//-- (so it only works for resources stored on upload and readback heaps).
		ok = m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't map a vertex buffer.");

		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		m_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
		m_vertexBufferView.StrideInBytes = sizeof(Vertex);
		m_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	//-- Create the constant buffer.
	{
		const UINT constantBufferSize = calculateConstantBufferByteSize(sizeof(GlobalConstBuffer)); //-- CB size is required to be 256-byte aligned.

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resDesc = CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize);

		assertIfFailed(m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBuffer)));

		//-- Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = constantBufferSize;
		m_device->CreateConstantBufferView(&cbvDesc, m_cbvHeap->GetCPUDescriptorHandleForHeapStart());

		//-- Map and initialize the constant buffer. We don't unmap this until the
		//-- app closes. Keeping things mapped for the lifetime of the resource is okay.
		CD3DX12_RANGE readRange(0, 0); //-- We do not intend to read from this resource on the CPU.
		assertIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin)));
		memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));
	}

	//-- Create and record the bundle.
	{
		ok = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_bundleCommands));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a bundle command list");

		m_bundleCommands->SetGraphicsRootSignature(m_rootSignature.Get());
		m_bundleCommands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_bundleCommands->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_bundleCommands->DrawInstanced(3, 1, 0, 0);
		assertIfFailed(m_bundleCommands->Close());
	}

	return true;
}


void Backend::release()
{
	waitForPreviousFrame();

	CloseHandle(m_fenceEvent);
	m_device.Reset();
}


void Backend::waitForPreviousFrame()
{
	//-- WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	//-- This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	//-- sample illustrates how to use fences for efficient resource usage and to
	//-- maximize GPU utilization.
	//-- ToDo: Reconsider later.

	//-- Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	HRESULT ok = m_graphicsCommandQueue->Signal(m_fence.Get(), fence);
	ENGINE_ASSERT_DEBUG(SUCCEEDED(ok), "Can't get a signal from a command queue");
	m_fenceValue++;

	//-- Wait until the previous frame is finished.
	//-- GetCompletedValue returns the value of the last fence met\executed by the GPU in the command queue.
	//-- If still no fence has been executed, this function returns 0.
	if (m_fence->GetCompletedValue() < fence)
	{
		ok = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}


void Backend::present()
{
	//-- ToDo: Update part.
	const float translationSpeed = 0.015f;
	const float offsetBounds = 1.25f;

	m_constantBufferData.offset.x += translationSpeed;
	if (m_constantBufferData.offset.x > offsetBounds)
	{
		m_constantBufferData.offset.x = -offsetBounds;
	}

	memcpy(m_pCbvDataBegin, &m_constantBufferData, sizeof(m_constantBufferData));

	//-- RENDER PART. TODO: MOVE OUT TO THE SYSTEMS.
	{
		//-- We use a single command allocator to manage the memory space where drawing commands for both buffers in the swap chain are recorded.
		//-- This implies that we need to flush the command queue before recording the commands to create and present a new frame,
		//-- as all commands are recorded in the same memory space regardless of the frame we are creating
		//-- - we can't overwrite commands still in use by the GPU, obviously.
		
		//-- Command list allocators can only be reset when the associated 
		//-- command lists have finished execution on the GPU; apps should use 
		//-- fences to determine GPU execution progress.
		HRESULT ok = m_commandAllocator->Reset();
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));

		//-- However, when ExecuteCommandList() is called on a particular command list,
		//-- that command list can then be reset at any time and must be before re-recording.
		ok = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
		m_commandList->SetPipelineState(m_pipelineState.Get()); //-- Or we can reset to this PSO.

		//-- Root Signature.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_cbvHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		m_commandList->SetGraphicsRootDescriptorTable(0, m_cbvHeap->GetGPUDescriptorHandleForHeapStart());

		//-- Indicate that the back buffer will be used as a render target.
		auto beginBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &beginBarriers);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		//-- Record commands.
		const float clearColor[] = { 0.2f, 0.8f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		m_commandList->ExecuteBundle(m_bundleCommands.Get());

		//-- Indicate that the back buffer will now be used to present.
		auto endBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &endBarriers);

		ok = m_commandList->Close(); //-- Command list must be close before submitting it to a command queue.
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
	}

	//-- Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//-- Present the frame.
	HRESULT ok = m_swapChain->Present(1, 0); //-- ToDo: Rework on v-sync param.
	ENGINE_ASSERT_DEBUG(SUCCEEDED(ok), "Can't present the frame.");

	waitForPreviousFrame();
}

//-- Binding slots are not physical blocks of memory or registers that a GPU can access to read descriptors.
//-- They are simple names (character strings) used to associate descriptors to resource declarations in shader programs

//-- GPUs have access to four types of memory :
//--
//-- 1) Dedicated video memory : this is memory reserved\local to the GPU(VRAM).
//--	It's where we allocate most of the resources accessed by the GPU(through the shader programs).
//--
//-- 2) Dedicated system memory : it is a part of the dedicated video memory
//--	It's allocated at boot time and used by the GPU for internal purposes.
//--	That is, we can't use it to allocate memory from our application.
//--
//-- 3) Shared system memory : this is CPU - visible GPU memory.
//--	Usually, it is a small part of the GPU local memory(VRAM) accessible by the CPU through the PCI - e bus,
//--	but the GPU can also use CPU system memory(RAM) as GPU memory if needed.
//--	Shared system memory is often used as a source in copy operations from shared to dedicated memory
//--	(that is, from CPU - accessible memory to GPU local memory) to prevent the GPU from accessing resources in memory via the PCI - e bus.
//--	It's write - combine memory from the CPU point of view, which means that write operations are buffered up and 
//--	executed in groups when the buffer is full, or when important events occur.This allows to speed up write operations,
//--	but read ones should be avoided as write - combine memory is uncached.
//--	This means that if you try to read this memory from your CPU application,
//--	the buffer that holds the write operations need to be flushed first, which makes reads from write - combine memory slow.
//--
//-- 4) CPU system memory : it's system memory(RAM) that, like shared system memory, can be accessed from both CPU and GPU.
//--	However, CPUs can read from this memory without problems as it is cached.
//--	On the other hand, GPUs need to access this memory through the PCI - e bus, which can be a bottleneck compared to
//--	the direct memory access of CPUs through the system memory bus.
//--
//-- If you have an integrated graphics card or use a software adapter, there is no distinction between the four memory types mentioned above.
//-- In that case, both the CPU and GPU will share the only memory type available: system memory (RAM).
//-- This implies that your GPU may have limited and slower memory access.

//-- When the CreateCommittedResource function is invoked, we need to specify the type of memory where space should be allocated for
//-- the resource we want to create.You can indicate this information in two ways : abstract and custom.
//-- In the abstract way, we have three types of memory heaps that allow abstraction from the current hardware.
//--
//-- 1) Default heap : memory that resides in dedicated video memory.
//-- 2) Upload heap : memory that resides in shared video memory.
//-- 3) Readback heap : memory that resides in CPU system memory.
//--
//-- Therefore, using the abstract approach, regardless of whether you have a discrete GPU(that is, a dedicated graphics card) or
//-- an integrated one, physical memory allocations are hidden from the programmer.

} //-- engine::render::d3d12.
