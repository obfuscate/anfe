#pragma once

#include <engine/services/service_manager.h>
#include <engine/reflection/common.h>

namespace engine
{

//-- ToDo: Add multi-windowing.
class WindowsService final : public Service<WindowsService>
{
public:
	WindowsService();
	~WindowsService() = default;

	bool initialize() override;
	void release() override;

	LLGL::Window& window()
	{
		ENGINE_ASSERT_DEBUG(m_swapChain != nullptr, "SwapChain is null");
		return LLGL::CastTo<LLGL::Window>(m_swapChain->GetSurface());
	}

private:
	LLGL::SwapChain* m_swapChain = nullptr;
};

} //-- engine.
