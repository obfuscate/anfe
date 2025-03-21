#include <engine/services/windows_service.h>
#include <engine/engine.h>
#include <engine/helpers.h>
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
META_REGISTRATION
{
	reflection::Service<WindowsService>("WindowsService")
		.cli({ "--wm"sv, "--res"sv})
	;
};

/*class WindowEventHandler : public LLGL::Window::EventListener
{
public:
	WindowEventHandler(LLGL::SwapChain* swapChain)
		: m_swapChain(swapChain)
	{
	}

	void OnResize(LLGL::Window& /*sender* /, const LLGL::Extent2D& clientAreaSize) override
	{
		m_swapChain->ResizeBuffers(clientAreaSize);
	}

	void OnUpdate(LLGL::Window& /*sender* /) override
	{
		__nop();
		// Re-draw frame
		/*if (app_.IsLoadingDone())
			app_.DrawFrame();* /
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
};*/


class SDLWindow : public WindowWrapper
{
public:
	SDLWindow(const WindowsService::WindowDesc& desc)
	{
		m_props = SDL_CreateProperties();

		//-- Common props.
		SDL_SetBooleanProperty(m_props, SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN, true);
		SDL_SetBooleanProperty(m_props, SDL_PROP_WINDOW_CREATE_FOCUSABLE_BOOLEAN, true);
		SDL_SetBooleanProperty(m_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, true);

		//-- Custom props.
		if (hasFlag(desc.flags, WindowsService::WindowFlags::Maximized))
		{
			SDL_SetBooleanProperty(m_props, SDL_PROP_WINDOW_CREATE_MAXIMIZED_BOOLEAN, true);
		}
		if (hasFlag(desc.flags, WindowsService::WindowFlags::Fullscreen))
		{
			SDL_SetBooleanProperty(m_props, SDL_PROP_WINDOW_CREATE_FULLSCREEN_BOOLEAN, true);
		}
		if (hasFlag(desc.flags, WindowsService::WindowFlags::Borderless))
		{
			SDL_SetBooleanProperty(m_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, true);
		}

		SDL_SetStringProperty(m_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, desc.title.data());
		SDL_SetNumberProperty(m_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, desc.width);
		SDL_SetNumberProperty(m_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, desc.height);

		m_window = SDL_CreateWindowWithProperties(m_props);
		ENGINE_ASSERT(m_window != nullptr, SDL_GetError());
	}

	~SDLWindow()
	{
		SDL_DestroyWindow(m_window);
		m_window = nullptr;

		SDL_DestroyProperties(m_props);
	}

	void* handle() override
	{
		return SDL_GetPointerProperty(SDL_GetWindowProperties(m_window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
	}

	void show() override
	{
		SDL_ShowWindow(m_window);
	}

	std::pair<uint16_t, uint16_t> size() const override
	{
		int width = 0, height = 0;
		SDL_GetWindowSize(m_window, &width, &height);

		return std::make_pair(width, height);
	}


private:
	SDL_Window* m_window = nullptr;
	SDL_PropertiesID m_props;
};

} //-- unnamed.


bool WindowsService::initialize()
{
	auto& parser = service<CLIService>().parser();
	std::string s;
	parser("--gapi", "dx12") >> s;

	WindowDesc desc;
	desc.title = fmt::format("AnFE ({})", s);
	//-- Handle window mode.
	parser("--wm") >> s;
	//-- ToDo: It shouldn't affect another window (e.g. docks in imgui).
	{
		if (s == "maximized")
		{
			desc.flags |= WindowFlags::Maximized;
		}
		else if (s == "fullscreen")
		{
			desc.flags |= WindowFlags::Fullscreen;
		}
		else if (s == "borderless")
		{
			desc.flags |= WindowFlags::Borderless;
		}
	}

	//-- Handle resolution.
	parser("--res", "1024x768") >> s;
	{
		auto pos = s.find("x");
		auto sWidth = s.substr(0, pos);
		auto sHeight = s.substr(pos + 1);

		desc.width = static_cast<uint16_t>(std::stoi(sWidth));
		desc.height = static_cast<uint16_t>(std::stoi(sHeight));
	}

	m_mainWindow = createWindow(desc);
	m_mainWindow->show();

	return true;
}


void WindowsService::release()
{
	m_windows.clear();
}


WindowWrapper* WindowsService::createWindow(const WindowDesc& desc)
{
	m_windows.push_back(std::make_unique<SDLWindow>(desc));
	auto* result = m_windows.back().get();

	//-- Listen for window/canvas events.
	service<InputService>().registerWindow(result);

	return result;
}

} //-- engine.
