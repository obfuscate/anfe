#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/services/assert_service.h>
#include <engine/services/cli_service.h>
#include <engine/services/input_service.h>
#include <engine/services/log_service.h>
#include <engine/services/render_service.h>
#include <engine/services/windows_service.h>
#include <engine/services/world_service.h>

namespace engine
{

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
	initialized &= m_serviceManager.add<WorldService>();
	initialized &= m_serviceManager.add<RenderService>();
	initialized &= m_serviceManager.add<WindowsService>();
	initialized &= m_serviceManager.add<InputService>();
	initialized &= m_serviceManager.add<WindowsService>();

	return initialized;
}


void Engine::run()
{
	m_run = true;
	while (m_run)
	{
		//-- On mobile platforms, if app has paused, the swap-chain might not be presentable until the app is resumed again
		/*if (!swapChain->IsPresentable())
		{
			std::this_thread::yield();
			continue;
		}*/

	/*#ifdef LLGL_OS_ANDROID
		if (input.KeyDown(LLGL::Key::BrowserBack))
			ANativeActivity_finish(ExampleBase::androidApp_->activity);
	#endif*/

		m_serviceManager.tick();
		//timer.MeasureTime();
		//UpdateScene(static_cast<float>(timer.GetDeltaTime()));
	}

	release();
}


void Engine::release()
{
	logger().info("Engine shutdown");
	m_serviceManager.release();
}

} //-- engine.
