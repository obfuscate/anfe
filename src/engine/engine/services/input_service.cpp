#include <engine/services/input_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/render_service.h>
#include <engine/services/renderdoc_service.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	reflection::Service<InputService>("InputService");
}

} //-- unnamed.


bool InputService::initialize()
{
	m_input = std::make_unique<LLGL::Input>();
	return true;
}


void InputService::release()
{
	m_input.reset(); //-- Should be released before RenderService.
}


void InputService::tick()
{
	const bool stop = !(LLGL::Surface::ProcessEvents() && !keyDown(LLGL::Key::Escape));
	if (stop)
	{
		engine().stop();
	}

	if (keyDown(LLGL::Key::F11))
	{
		service<render::RenderDocService>().captureFrames("", 1);
	}
}


void InputService::postTick()
{
	m_input->Reset();
}

} //-- engine.
