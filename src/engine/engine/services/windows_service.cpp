#include <engine/services/windows_service.h>
#include <engine/engine.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/render_service.h>

namespace engine
{

using namespace std::string_view_literals;

namespace
{
RTTR_REGISTRATION
{
	reflection::Service<WindowsService>("WindowsService")
		.cli({ "--wm"sv, "--res"sv})
	;
};

class WindowEventHandler : public LLGL::Window::EventListener
{
public:
	WindowEventHandler(LLGL::SwapChain* swapChain)
		: m_swapChain(swapChain)
	{
	}

	void OnResize(LLGL::Window& sender, const LLGL::Extent2D& clientAreaSize) override
	{
		if (clientAreaSize.width >= 4 && clientAreaSize.height >= 4)
		{
			const auto& resolution = clientAreaSize;

			// Update swap buffers
			m_swapChain->ResizeBuffers(resolution);
			m_swapChain->Present();

			//-- Notify application about resize event
			//app_.OnResize(resolution);

			// Re-draw frame
			//if (app_.IsLoadingDone())
			//	app_.DrawFrame();
		}
	}


	void OnUpdate(LLGL::Window& sender) override
	{
		__nop();
		// Re-draw frame
		/*if (app_.IsLoadingDone())
			app_.DrawFrame();*/
	}

private:
	LLGL::SwapChain* m_swapChain = nullptr;
};

} //-- unnamed.


WindowsService::WindowsService()
	: Service()
	/*, m_mainWindow(nullptr, &SDL_DestroyWindow)*/ { }


bool WindowsService::initialize()
{
	auto& parser = engine::instance().serviceManager().get<CLIService>().parser();
	std::string s;

	//-- Handle window mode.
	parser("--wm") >> s;
	{
		if (s == "maximized")
		{
			//flags |= SDL_WINDOW_MAXIMIZED;
		}
		else if (s == "fullscreen")
		{
			//flags |= SDL_WINDOW_FULLSCREEN;
		}
	}

	//-- Handle resolution.
	int32_t width = 1024;
	int32_t height = 768;
	parser("--res", "1024x768") >> s;
	{
		auto pos = s.find("x");
		auto sWidth = s.substr(0, pos);
		auto sHeight = s.substr(pos + 1);
		//-- ToDo: scanf of another solution?
		width = std::stoi(sWidth);
		height = std::stoi(sHeight);
	}

	auto& rs = instance().serviceManager().get<RenderService>();
	m_swapChain = rs.swapChain();

	// Set window title
	auto& wnd = window();

	auto gapiName = rttr::type::get<GraphicsAPI>().get_enumeration().value_to_name(rs.gapi());
	wnd.SetTitle(fmt::format("AnFE ({})", gapiName));

	//-- Add window resize listener
	wnd.AddEventListener(std::make_shared<WindowEventHandler>(m_swapChain));
	//-- By some reasons it doesn't work. Perhaps need to call Present();
	wnd.PostResize(LLGL::Extent2D(width, height));
	//m_swapChain->ResizeBuffers();

	wnd.Show();

	return true;
}


void WindowsService::release()
{

}

} //-- engine.
