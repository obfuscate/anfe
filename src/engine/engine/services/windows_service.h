#pragma once

#include <engine/services/service_manager.h>
#include <engine/reflection/common.h>
#include <engine/utils/enum.h>

namespace engine
{

class WindowWrapper
{
public:
	WindowWrapper() = default;
	virtual ~WindowWrapper() = default;

	virtual void* handle() = 0;
	virtual void show() = 0;

	virtual auto size() const -> std::pair<uint16_t, uint16_t> = 0;
};


class WindowsService final : public Service<WindowsService>
{
public:
	enum class WindowFlags : uint8_t
	{
		None,
		Fullscreen = 1 << 0,
		Maximized = 1 << 1,
		Borderless = 1 << 2
	};

	struct WindowDesc
	{
		std::string title;
		uint16_t width = 0u;
		uint16_t height = 0u;
		WindowFlags flags = WindowFlags::None;
	};

public:
	WindowsService() = default;
	~WindowsService() = default;

	bool initialize();
	void release() override;

	WindowWrapper* createWindow(const WindowDesc& desc);
	WindowWrapper* mainWindow() { return m_mainWindow; }

private:
	std::vector<std::unique_ptr<WindowWrapper>> m_windows;
	WindowWrapper* m_mainWindow = nullptr;
};

DEFINE_ENUM_CLASS_BITWISE_OPERATORS(WindowsService::WindowFlags)

} //-- engine.
