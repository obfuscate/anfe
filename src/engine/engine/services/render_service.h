#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

enum class GraphicsAPI : uint8_t
{
	DirectX12 = 0,
	Vulkan,
	Unknown
};

class RenderService final : public Service<RenderService>
{
public:
	RenderService() = default;
	~RenderService() = default;

	bool initialize() override;
	void release() override;

	GraphicsAPI gapi() const { return m_gapi; }

	LLGL::SwapChain* swapChain() { return m_swapChain; }
	LLGL::RenderingDebugger* debugger() { return m_debugger.get(); }

private:
	using DebuggerPtr = std::unique_ptr<LLGL::RenderingDebugger>;

	LLGL::RenderSystemPtr m_renderer;
	DebuggerPtr m_debugger;
	LLGL::SwapChain* m_swapChain = nullptr;
	GraphicsAPI m_gapi = GraphicsAPI::Unknown;
};

} //-- engine.
