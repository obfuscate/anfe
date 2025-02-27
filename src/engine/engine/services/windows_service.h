#pragma once
#include <engine/export.h>
#include <engine/services/service.h>

#include <SDL3/SDL.h>

namespace engine
{

//-- ToDo: Add multi-windowing.
class WindowsService final : public IService
{
public:
	WindowsService();
	~WindowsService() = default;

	bool initialize() override;

private:
	using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

	WindowPtr m_mainWindow;
};

} //-- engine.
