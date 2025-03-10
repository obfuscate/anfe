#include <engine/services/windows_service.h>
#include <engine/engine.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/imgui_service.h>
#include <engine/services/input_service.h>
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
		m_swapChain->ResizeBuffers(clientAreaSize);
	}

	void OnUpdate(LLGL::Window& sender) override
	{
		__nop();
		// Re-draw frame
		/*if (app_.IsLoadingDone())
			app_.DrawFrame();*/
	}

	void OnLocalMotion(LLGL::Window& sender, const LLGL::Offset2D& position) override
	{
		auto* context = static_cast<ImGUIService::Backend::WindowContext*>(sender.GetUserData());
		if (context != nullptr)
		{
			context->mousePosInWindow = position;
		}
	}

private:
	LLGL::SwapChain* m_swapChain = nullptr;
};

} //-- unnamed.


WindowsService::WindowsService()
	: Service() { }


bool WindowsService::initialize()
{
	auto& parser = engine::engine().serviceManager().get<CLIService>().parser();
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

	auto& rs = engine().serviceManager().get<RenderService>();
	auto gapiName = rttr::type::get<GraphicsAPI>().get_enumeration().value_to_name(rs.gapi());
	m_mainWindow = createWindow(fmt::format("AnFE ({})", gapiName), static_cast<uint16_t>(width), static_cast<uint16_t>(height));
	m_mainWindow->window().Show();

	return true;
}


void WindowsService::release()
{

}


WindowsService::WindowWrapper* WindowsService::createWindow(std::string_view name, const uint16_t width, const uint16_t height)
{
	auto& rs = engine().serviceManager().get<RenderService>();
	auto* swapChain = rs.createSwapChain(width, height, name);

	m_windows.push_back(std::make_unique<WindowWrapper>(swapChain));
	auto* wrapper = m_windows.back().get();
	//-- Set window title
	auto& wnd = wrapper->window();

	wnd.SetTitle(name.data());

	//-- Add window resize listener
	//-- ToDo: Create only one window event handler and reuse it. User data might be get from LLGL::Window::GetUserData.
	wnd.AddEventListener(std::make_shared<WindowEventHandler>(wrapper->swapChain));

	//-- Listen for window/canvas events.
	engine().serviceManager().get<InputService>().registerWindow(swapChain->GetSurface());

	return wrapper;
}

} //-- engine.
