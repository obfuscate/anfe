#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

class WindowWrapper; //-- forward declaration.

class InputService final : public Service<InputService>
{
public:
	InputService() = default;
	~InputService() = default;

	bool initialize();
	void release() override;

	void tick() override;
	void postTick() override;

	bool keyDown(const SDL_Scancode key) const { return false; /*m_input->KeyDown(key);*/ }
	bool keyUp(const SDL_Scancode key) const { return false; /* m_input->KeyUp(key);*/ }

	void registerWindow(const WindowWrapper*) { /*m_input->Listen(window);*/ }

private:
	//-- Because RenderService::release() unload the RenderSystem we have to destroy LLGL::Inputr object before this action.
	
};

} //-- engine.
