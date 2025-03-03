#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/services/cli_service.h>
#include <engine/services/log_service.h>
#include <engine/services/windows_service.h>

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
	initialized &= m_serviceManager.add<CLIService>(argc, argv);
	initialized &= m_serviceManager.add<LogService>();
	initialized &= m_serviceManager.add<WindowsService>();

	return initialized;
}

void Engine::run()
{
	//-- ToDo: Reconsider later.
	bool run = true;
	while (run) {
		for (SDL_Event e; SDL_PollEvent(&e);) {
			if (e.type == SDL_EVENT_QUIT) {
				run = false;
			}
		}
	}

	logger().info("Engine shutdown");
	m_serviceManager.release();
}

} //-- engine.
