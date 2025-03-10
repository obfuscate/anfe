#pragma once

#include <engine/services/service_manager.h>
#include <engine/reflection/common.h>

namespace engine
{

//-- ToDo: Add multi-windowing.
class WindowsService final : public Service<WindowsService>
{
public:
	class WindowWrapper
	{
	public:
		WindowWrapper(LLGL::SwapChain* swapChain)
			: swapChain(swapChain) { }

		LLGL::Window& window()
		{
			ENGINE_ASSERT_DEBUG(swapChain != nullptr, "SwapChain is null");
			return LLGL::CastTo<LLGL::Window>(swapChain->GetSurface());
		}

	public:
		LLGL::SwapChain* swapChain = nullptr;
	};

public:
	WindowsService();
	~WindowsService() = default;

	bool initialize() override;
	void release() override;

	WindowWrapper* createWindow(std::string_view name, const uint16_t width, const uint16_t height);
	WindowWrapper* mainWindow() { return m_mainWindow; }

private:
	std::vector<std::unique_ptr<WindowWrapper>> m_windows;
	WindowWrapper* m_mainWindow;
};

} //-- engine.
