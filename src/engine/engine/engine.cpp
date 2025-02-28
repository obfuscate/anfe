#include <engine/engine.h>

#include <engine/services/windows_service.h>
#include <engine/services/cli_service.h>

namespace engine
{

bool Engine::initialize(const int argc, const char* const argv[])
{
	m_services.push_back(std::make_unique<CLIService>(argc, argv));
	m_services.push_back(std::make_unique<WindowsService>());

	bool initialized = true;
	for (auto& service : m_services)
	{
		initialized &= service->initialize();
	}

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
}

} //-- engine.
