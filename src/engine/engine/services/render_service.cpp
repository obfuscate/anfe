#include <engine/services/render_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/input_service.h>
#include <engine/services/windows_service.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	rttr::registration::enumeration<GraphicsAPI>("GraphicsAPI")
	(
		rttr::value("dx12", GraphicsAPI::DirectX12)
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
	m_imguiCommandList = m_renderer->CreateCommandBuffer(cmdBufferDesc);

	// Get command queue
	m_commandQueue = m_renderer->GetCommandQueue();
}


void RenderService::CommandListPool::submit()
{
	m_commandQueue->Submit(*m_commandList);
	m_commandQueue->Submit(*m_imguiCommandList);
}


bool RenderService::initialize()
{
	auto& cli = engine().serviceManager().get<CLIService>().parser();

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

	//-- Print renderer information
	const LLGL::RendererInfo& info = m_renderer->GetRendererInfo();

	logger().info(fmt::format("[RenderService]: render system:\n"
		"  renderer:           {}\n"
		"  device:             {}\n"
		"  vendor:             {}\n"
		"  shading language:   {}\n"
		"\n",
		info.rendererName.c_str(),
		info.deviceName.c_str(),
		info.vendorName.c_str(),
		info.shadingLanguageName.c_str())
	);

	m_commandListPool.initialize(m_renderer.get());

	return true;
}


void RenderService::release()
{
	for (auto swapChain : m_swapChains)
	{
		m_renderer->Release(*swapChain);
	}
	m_swapChains.clear();

	m_renderer.reset();
	m_debugger.reset();
	LLGL::RenderSystem::Unload(std::move(m_renderer));
}


void RenderService::postTick()
{
	auto& sm = engine().serviceManager();
	auto& is = sm.get<InputService>();

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

	//-- Present the result on the screen - cannot be explicitly invoked on mobile platforms (#ifndef LLGL_OS_IOS)
	for (auto swapChain : m_swapChains)
	{
		swapChain->Present();
	}

	//-- On desktop platforms, we also want to quit the app if the close button has been pressed (#ifndef LLGL_MOBILE_PLATFORM)
	if (!isAnyWindowOpen())
	{
		engine().stop();
		return;
	}
}


LLGL::SwapChain* RenderService::createSwapChain(const uint16_t width, const uint16_t height, std::string_view debugName)
{
	LLGL::SwapChainDescriptor swapChainDesc;
	{
		swapChainDesc.debugName = debugName.data();
		//-- ToDo: DO move scaleResolutionForDisplay out?
		swapChainDesc.resolution = scaleResolutionForDisplay(LLGL::Extent2D(width, height), LLGL::Display::GetPrimary());
		//-- ToDo.
		swapChainDesc.samples = std::min<std::uint32_t>(1, m_renderer->GetRenderingCaps().limits.maxColorBufferSamples);
		swapChainDesc.resizable = true;
		swapChainDesc.depthBits = 0; //-- Disable depth buffer.
		swapChainDesc.stencilBits = 0; //-- Disable stencil buffer.
		swapChainDesc.swapBuffers = 2; //-- explicit setup.
	}
	auto swapChain = m_renderer->CreateSwapChain(swapChainDesc);

	swapChain->SetVsyncInterval(1); //-- ToDo: Pass via cli or use settings.

	m_swapChains.push_back(swapChain);

	const LLGL::Extent2D swapChainRes = swapChain->GetResolution();
	logger().info(fmt::format("[RenderService]: create new swap-chain:\n"
		"  resolution:         {} x {}\n"
		"  samples:            {}\n"
		"  colorFormat:        {}\n"
		"  depthStencilFormat: {}\n"
		"\n",
		swapChainRes.width,
		swapChainRes.height,
		swapChain->GetSamples(),
		LLGL::ToString(swapChain->GetColorFormat()),
		LLGL::ToString(swapChain->GetDepthStencilFormat()))
	);

	return swapChain;
}


bool RenderService::isAnyWindowOpen()
{
	for (LLGL::SwapChain* swapChain : m_swapChains)
	{
		auto& window = LLGL::CastTo<LLGL::Window>(swapChain->GetSurface());
		if (window.IsShown())
		{
			return true;
		}
	}
	return false;
}

} //-- engine.
