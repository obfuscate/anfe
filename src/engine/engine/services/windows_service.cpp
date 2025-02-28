#include <engine/services/windows_service.h>
#include <engine/engine.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>

namespace engine
{

using namespace std::string_view_literals;

namespace
{
RTTR_REGISTRATION
{
	reflection::Service<WindowsService>("WindowsService")
		.cli({ "-wm"sv, "-res"sv})
	;
};

} //-- unnamed.

WindowsService::WindowsService()
	: Service()
	, m_mainWindow(nullptr, &SDL_DestroyWindow) {}

bool WindowsService::initialize()
{
	int32_t width = 1024;
	int32_t height = 768;
	SDL_WindowFlags flags = 0;

	auto& parser = engine::instance().serviceManager().get<CLIService>().parser();
	std::string s;

	//-- Handle window mode.
	parser("-wm") >> s;
	{
		if (s == "maximized")
		{
			flags |= SDL_WINDOW_MAXIMIZED;
		}
		else if (s == "fullscreen")
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}
	}

	//-- Handle resolution.
	parser("-res", "1024x768") >> s;
	{
		auto pos = s.find("x");
		auto sWidth = s.substr(0, pos);
		auto sHeight = s.substr(pos + 1);
		//-- ToDo: scanf of another solution?
		width = std::stoi(sWidth);
		height = std::stoi(sHeight);
	}

	m_mainWindow.reset(SDL_CreateWindow("AnFE", width, height, flags));

	bool initialized = m_mainWindow != nullptr;
	return initialized;
}

void WindowsService::release()
{
	m_mainWindow.reset();
}

} //-- engine.
