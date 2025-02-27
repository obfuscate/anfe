#include <engine/services/windows_service.h>

namespace engine
{

WindowsService::WindowsService()
	: IService()
	, m_mainWindow(nullptr, &SDL_DestroyWindow) {}

bool WindowsService::initialize()
{
	int32_t width = 1024;
	int32_t height = 768;
	SDL_WindowFlags flags = 0;

	m_mainWindow.reset(SDL_CreateWindow("AnFE", width, height, flags));

	bool initialized = m_mainWindow != nullptr;
	return initialized;
}

} //-- engine.
