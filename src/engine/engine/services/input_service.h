#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

class InputService final : public Service<InputService>
{
public:
	InputService() = default;
	~InputService() = default;

	bool initialize() override;
	void release() override;

	void tick() override;
	void postTick() override;

	bool keyDown(const LLGL::Key key) const { return m_input->KeyDown(key); }
	bool keyUp(const LLGL::Key key) const { return m_input->KeyUp(key); }

	void registerWindow(LLGL::Surface& window) { m_input->Listen(window); }

private:
	//-- Because RenderService::release() unload the RenderSystem we have to destroy LLGL::Inputr object before this action.
	using InputPtr = std::unique_ptr<LLGL::Input>;
	InputPtr m_input;
};

} //-- engine.
