#pragma once
#include <engine/export.h>
#include <engine/pch.h>
#include <engine/services/service_manager.h>
#include <engine/reflection/common.h>

#include <SDL3/SDL.h>

namespace engine
{

//-- ToDo: Add multi-windowing.
class WindowsService final : public Service<WindowsService>
{
public:
	WindowsService();
	~WindowsService() = default;

	bool initialize() override;
	void release() override;

private:
	using WindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;

	WindowPtr m_mainWindow;
};

} //-- engine.
