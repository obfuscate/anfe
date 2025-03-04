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
	using CommandList = LLGL::CommandBuffer;

	class CommandListPool
	{
	public:
		CommandList* requestCommandList();

	private:
		void initialize(LLGL::RenderSystem* renderer);
		void submit();

		friend class RenderService;

	private:
		LLGL::RenderSystem* m_renderer = nullptr;
		LLGL::CommandBuffer* m_commandList = nullptr;
		LLGL::CommandQueue* m_commandQueue = nullptr;
	};

public:
	RenderService() = default;
	~RenderService() = default;

	bool initialize() override;
	void release() override;

	void tick() override;

	GraphicsAPI gapi() const { return m_gapi; }

	LLGL::SwapChain* swapChain() { return m_swapChain; }
	LLGL::RenderingDebugger* debugger() { return m_debugger.get(); }

	CommandListPool& commandListPool() { return m_commandListPool; }

private:
	using DebuggerPtr = std::unique_ptr<LLGL::RenderingDebugger>;

	CommandListPool m_commandListPool;
	LLGL::RenderSystemPtr m_renderer;
	DebuggerPtr m_debugger;
	LLGL::SwapChain* m_swapChain = nullptr;
	GraphicsAPI m_gapi = GraphicsAPI::Unknown;
};

} //-- engine.
