#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/services/assert_service.h>
#include <engine/services/cli_service.h>
#include <engine/services/editor_service.h>
#include <engine/services/imgui_service.h>
#include <engine/services/input_service.h>
#include <engine/services/log_service.h>
#include <engine/services/renderdoc_service.h>
#include <engine/services/render_service.h>
#include <engine/services/windows_service.h>
#include <engine/services/world_service.h>

namespace engine
{
using namespace std::string_view_literals;

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
	initialized &= m_serviceManager.add<render::RenderDocService>(); //-- Should be initialized before any GAPI initialization.
	initialized &= m_serviceManager.add<RenderService>();
	initialized &= m_serviceManager.add<InputService>(); //-- Should be before WindowsService, because it registers windows as listeners.
	initialized &= m_serviceManager.add<WindowsService>();
	initialized &= m_serviceManager.add<ImGUIService>(); //-- Should be after RenderService, because it uses RS to retrive some handles. Destroy before RenderService.
	initialized &= m_serviceManager.add<EditorService>();

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

		//m_serviceManager.tick();

		auto& sm = m_serviceManager;
		//-- ToDo: Add hints to reflection and use some graph builder to auto-build order.
		{
			sm.get<InputService>().tick(); //-- ToDo: Should be before WorldService or service where we handle input.
			sm.get<ImGUIService>().tick(); //-- Should be before any service which may use ImGui.
			sm.get<EditorService>().tick();
			sm.get<WorldService>().tick();
			//sm.get<RenderService>().tick();
			//sm.get<WindowsService>().tick();
		}

		//-- post tick.
		{
			sm.get<ImGUIService>().postTick(); //-- Should be after any service which may use ImGui and before RenderService.
			sm.get<RenderService>().postTick();
			sm.get<InputService>().postTick();
		}
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
