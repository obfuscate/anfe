#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/services/assert_service.h>
#include <engine/services/cli_service.h>
#include <engine/services/log_service.h>
#include <engine/services/render_service.h>
#include <engine/services/windows_service.h>

namespace engine
{

Engine::~Engine()
{
	release();
}

Engine& Engine::instance()
{
	static Engine s_engine;
	return s_engine;
}


bool Engine::initialize(const int argc, const char* const argv[])
{
	//-- ToDo: Think about ordering in plugins.
	bool initialized = true;
	initialized &= m_serviceManager.add<AssertService>();
	initialized &= m_serviceManager.add<CLIService>(argc, argv);
	initialized &= m_serviceManager.add<LogService>();
	initialized &= m_serviceManager.add<RenderService>();
	initialized &= m_serviceManager.add<WindowsService>();

	//-- Listen for window/canvas events.
	m_input.Listen(m_serviceManager.get<RenderService>().swapChain()->GetSurface());

	return initialized;
}


void Engine::run()
{
	auto& rs = m_serviceManager.get<RenderService>();
	auto& ws = m_serviceManager.get<WindowsService>();

	auto swapChain = rs.swapChain();
	auto& window = ws.window();
	while (LLGL::Surface::ProcessEvents() && !m_input.KeyDown(LLGL::Key::Escape))
	{
	#ifndef LLGL_MOBILE_PLATFORM
		//-- On desktop platforms, we also want to quit the app if the close button has been pressed
		if (window.HasQuit())
		{
			break;
		}
	#endif

		//-- On mobile platforms, if app has paused, the swap-chain might not be presentable until the app is resumed again
		if (!swapChain->IsPresentable())
		{
			std::this_thread::yield();
			continue;
		}

	#ifdef LLGL_OS_ANDROID
		if (input.KeyDown(LLGL::Key::BrowserBack))
			ANativeActivity_finish(ExampleBase::androidApp_->activity);
	#endif

		mainLoop();
	}
}


void Engine::release()
{
	logger().info("Engine shutdown");
	m_serviceManager.release();
}


void Engine::mainLoop()
{
	auto& rs = m_serviceManager.get<RenderService>();
	auto debugger = rs.debugger();
	//-- Update profiler (if debugging is enabled)
	if (debugger)
	{
		LLGL::FrameProfile frameProfile;
		debugger->FlushProfile(&frameProfile);

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
	if (m_input.KeyDown(LLGL::Key::F5))
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

	//-- Draw current frame
	drawFrame();

	m_input.Reset();
}

void Engine::drawFrame()
{
	//-- Real rendering code is here...
	// Draw frame in respective example project
	// Update scene by user input
	/*timer.MeasureTime();
	UpdateScene(static_cast<float>(timer.GetDeltaTime()));

	commands->Begin();
	{
		// Bind common input assembly
		commands->SetVertexBuffer(*vertexBuffer);

		// Render everything directly into the swap-chain
		commands->BeginRenderPass(*swapChain);
		{
			commands->Clear(LLGL::ClearFlags::ColorDepth, backgroundColor);
			commands->SetViewport(swapChain->GetResolution());
			RenderScene();
		}
		commands->EndRenderPass();
	}
	commands->End();
	commandQueue->Submit(*commands);*/

#ifndef LLGL_OS_IOS
	// Present the result on the screen - cannot be explicitly invoked on mobile platforms
	m_serviceManager.get<RenderService>().swapChain()->Present();
#endif
}

} //-- engine.
