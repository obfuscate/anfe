#include <engine/services/render_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/input_service.h>
#include <engine/services/windows_service.h>

#include <LLGL/Utils/TypeNames.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	rttr::registration::enumeration<GraphicsAPI>("GraphicsAPI")
	(
		rttr::value("dx12", GraphicsAPI::DirectX12),
		rttr::value("vk", GraphicsAPI::Vulkan)
	);

	reflection::Service<RenderService>("RenderService")
		.cli({"-rdl", "--gapi", "-rdboe"})
	;
}


LLGL::Extent2D scaleResolution(const LLGL::Extent2D& res, float scale)
{
	const float wScaled = static_cast<float>(res.width) * scale;
	const float hScaled = static_cast<float>(res.height) * scale;
	return LLGL::Extent2D
	{
		static_cast<std::uint32_t>(wScaled + 0.5f),
		static_cast<std::uint32_t>(hScaled + 0.5f)
	};
}

LLGL::Extent2D scaleResolutionForDisplay(const LLGL::Extent2D& res, const LLGL::Display* display)
{
	if (display != nullptr)
		return scaleResolution(res, display->GetScale());
	else
		return res;
}

} //-- unnamed.


RenderService::CommandList* RenderService::CommandListPool::requestCommandList()
{
	return m_commandList;
}


void RenderService::CommandListPool::initialize(LLGL::RenderSystem* renderer)
{
	m_renderer = renderer;

	// Create command buffer
	LLGL::CommandBufferDescriptor cmdBufferDesc;
	{
		cmdBufferDesc.debugName = "Commands";
		//if (g_Config.immediateSubmit)
		//	cmdBufferDesc.flags = LLGL::CommandBufferFlags::ImmediateSubmit;
	}
	m_commandList = m_renderer->CreateCommandBuffer(cmdBufferDesc);

	// Get command queue
	m_commandQueue = m_renderer->GetCommandQueue();
}


void RenderService::CommandListPool::submit()
{
	m_commandQueue->Submit(*m_commandList);
}


bool RenderService::initialize()
{
	auto& cli = instance().serviceManager().get<CLIService>().parser();

	//-- Select Graphics API.
	m_gapi = GraphicsAPI::Unknown;

	std::string stringGAPI;
	cli("--gapi", "dx12") >> stringGAPI;

	rttr::enumeration type = rttr::type::get<GraphicsAPI>().get_enumeration();
	rttr::variant var = type.name_to_value(stringGAPI);
	if (var.is_valid())
	{
		m_gapi = var.get_value<GraphicsAPI>();
	}

	ENGINE_ASSERT(m_gapi != GraphicsAPI::Unknown, "Invalid Graphics API!");
	//-- Forward all log reports to the standard output stream for errors
	//LLGL::Log::RegisterCallbackStd();

	LLGL::RenderSystemDescriptor rendererDesc;
	rendererDesc.flags = 0;
	if (cli["-rdl"])
	{
		m_debugger.reset(new LLGL::RenderingDebugger);

		rendererDesc.debugger = m_debugger.get();
		rendererDesc.flags |= LLGL::RenderSystemFlags::DebugDevice;
	}
	if (cli["-rdboe"])
	{
		rendererDesc.flags |= LLGL::RenderSystemFlags::DebugBreakOnError;
	}

	switch (m_gapi)
	{
	case GraphicsAPI::DirectX12:
	{
		rendererDesc.moduleName = "Direct3D12";
		break;
	}
	case GraphicsAPI::Vulkan:
	{
		rendererDesc.moduleName = "Vulkan";
		break;
	}
	default:
	{
		ENGINE_FAIL("Incorrect Graphics API");
	}
	}

	LLGL::Report report;
	m_renderer = LLGL::RenderSystem::Load(rendererDesc, &report);
	if (m_renderer == nullptr)
	{
		ENGINE_FAIL(fmt::format("Couldn't create a renderer. Error: {}", report.GetText()));
	}

	// Create swap-chain
	LLGL::SwapChainDescriptor swapChainDesc;
	{
		swapChainDesc.debugName = "SwapChain";
		swapChainDesc.resolution = scaleResolutionForDisplay(LLGL::Extent2D(1024, 768), LLGL::Display::GetPrimary());
		//-- ToDo.
		swapChainDesc.samples = std::min<std::uint32_t>(1, m_renderer->GetRenderingCaps().limits.maxColorBufferSamples);
		swapChainDesc.resizable = true;
		swapChainDesc.depthBits = 0; //-- Disable depth buffer.
		swapChainDesc.stencilBits = 0; //-- Disable stencil buffer.
	}
	m_swapChain = m_renderer->CreateSwapChain(swapChainDesc);

	m_swapChain->SetVsyncInterval(1); //-- ToDo: Pass via cli.

	//-- Print renderer information
	const LLGL::RendererInfo& info = m_renderer->GetRendererInfo();
	const LLGL::Extent2D swapChainRes = m_swapChain->GetResolution();

	logger().info(fmt::format("render system:\n"
		"  renderer:           {}\n"
		"  device:             {}\n"
		"  vendor:             {}\n"
		"  shading language:   {}\n"
		"\n"
		"swap-chain:\n"
		"  resolution:         {} x {}\n"
		"  samples:            {}\n"
		"  colorFormat:        {}\n"
		"  depthStencilFormat: {}\n"
		"\n",
		info.rendererName.c_str(),
		info.deviceName.c_str(),
		info.vendorName.c_str(),
		info.shadingLanguageName.c_str(),
		swapChainRes.width,
		swapChainRes.height,
		m_swapChain->GetSamples(),
		LLGL::ToString(m_swapChain->GetColorFormat()),
		LLGL::ToString(m_swapChain->GetDepthStencilFormat()))
	);
	//samples_ = swapChain->GetSamples();

	//m_renderer->GetNativeHandle();

	m_commandListPool.initialize(m_renderer.get());

	return true;
}


void RenderService::release()
{
	m_renderer->Release(*m_swapChain);
	m_swapChain = nullptr;

	m_renderer.reset();
	m_debugger.reset();
	LLGL::RenderSystem::Unload(std::move(m_renderer));
}


void RenderService::tick()
{
	auto& sm = instance().serviceManager();
	auto& is = sm.get<InputService>();
	auto& ws = sm.get<WindowsService>();

#ifndef LLGL_MOBILE_PLATFORM
	//-- On desktop platforms, we also want to quit the app if the close button has been pressed
	if (ws.window().HasQuit())
	{
		instance().stop();
		return;
	}
#endif
	//-- Update profiler (if debugging is enabled)
	if (m_debugger)
	{
		LLGL::FrameProfile frameProfile;
		m_debugger->FlushProfile(&frameProfile);

		/*if (showTimeRecords_)
		{
			LLGL::Log::Printf(
				"\n"
				"FRAME TIME RECORDS:\n"
				"-------------------\n"
			);
			const double invTicksFreqMS = 1000.0 / LLGL::Timer::Frequency();
			for (const LLGL::ProfileTimeRecord& rec : frameProfile.timeRecords)
				LLGL::Log::Printf("%s: GPU time: %" PRIu64 " ns\n", rec.annotation.c_str(), rec.elapsedTime);

			debuggerObj_->SetTimeRecording(false);
			showTimeRecords_ = false;

			// Write frame profile to JSON file to be viewed in Google Chrome's Trace Viewer
			const char* frameProfileFilename = "LLGL.trace.json";
			WriteFrameProfileToJsonFile(frameProfile, frameProfileFilename);
			LLGL::Log::Printf("Saved frame profile to file: %s\n", frameProfileFilename);
		}
		else if (m_input.KeyDown(LLGL::Key::F1))
		{
			debuggerObj_->SetTimeRecording(true);
			showTimeRecords_ = true;
		}*/
	}

	//-- Check to switch to fullscreen
	if (is.keyDown(LLGL::Key::F5))
	{
		/*if (LLGL::Display* display = swapChain->GetSurface().FindResidentDisplay())
		{
			fullscreen_ = !fullscreen_;
			if (fullscreen_)
				swapChain->ResizeBuffers(display->GetDisplayMode().resolution, LLGL::ResizeBuffersFlags::FullscreenMode);
			else
				swapChain->ResizeBuffers(initialResolution_, LLGL::ResizeBuffersFlags::WindowedMode);
		}*/
	}

	m_commandListPool.submit();

#ifndef LLGL_OS_IOS
	// Present the result on the screen - cannot be explicitly invoked on mobile platforms
	m_swapChain->Present();
#endif
}

} //-- engine.
