#include <engine/services/input_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/render_service.h>
#include <engine/services/renderdoc_service.h>

namespace engine
{

namespace
{

META_REGISTRATION
{
	reflection::Service<InputService>("InputService");
}

} //-- unnamed.


bool InputService::initialize()
{
	//m_input = std::make_unique<LLGL::Input>();
	return true;
}


void InputService::release()
{
	//m_input.reset(); //-- Should be released before RenderService.
}


void InputService::tick()
{
	//-- ToDo: Register callbacks and handle events in specific places.
	SDL_Event windowEvent;
	if (SDL_PollEvent(&windowEvent))
	{
		switch (windowEvent.type)
		{
		case SDL_EVENT_QUIT:
		{
			engine().stop();
			break;
		}
		case SDL_EVENT_KEY_DOWN:
		{
			if (windowEvent.key.key == SDLK_ESCAPE)
			{
				engine().stop();
			}
			if (windowEvent.key.key == SDLK_F12)
			{
				if (auto* rdoc = findService<render::RenderDocService>())
				{
					rdoc->captureFrames("", 1);
				}
			}
			break;
		}
		}
	}
}


void InputService::postTick()
{
	//m_input->Reset();
}

} //-- engine.
