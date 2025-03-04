#include <engine/services/input_service.h>
#include <engine/engine.h>
#include <engine/reflection/registration.h>
#include <engine/services/render_service.h>

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
	//-- Listen for window/canvas events.
	m_input = std::make_unique<LLGL::Input>();
	m_input->Listen(instance().serviceManager().get<RenderService>().swapChain()->GetSurface());

	return true;
}


void InputService::release()
{
	m_input.reset(); //-- Should be released before RenderService.
}


void InputService::tick()
{
	const bool stop = !(LLGL::Surface::ProcessEvents() && !m_input->KeyDown(LLGL::Key::Escape));
	if (stop)
	{
		instance().stop();
	}
}

} //-- engine.
