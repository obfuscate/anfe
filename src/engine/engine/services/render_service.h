#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

enum class GraphicsAPI : uint8_t
{
	DirectX12 = 0,
	//-- ToDo: Add Vulkan later.
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
		//-- ToDo: Remove later.
		CommandList* imguiCommandList() { return m_imguiCommandList; }

	private:
		void initialize(LLGL::RenderSystem* renderer);
		void submit();

		friend class RenderService;

	private:
		LLGL::RenderSystem* m_renderer = nullptr;
		LLGL::CommandBuffer* m_commandList = nullptr;
		LLGL::CommandBuffer* m_imguiCommandList = nullptr;
		LLGL::CommandQueue* m_commandQueue = nullptr;
	};

public:
	RenderService() = default;
	~RenderService() = default;

	bool initialize() override;
	void release() override;

	void postTick() override;

	GraphicsAPI gapi() const { return m_gapi; }

	LLGL::RenderSystem* renderer() { return m_renderer.get(); }
	LLGL::RenderingDebugger* debugger() { return m_debugger.get(); }
	
	CommandListPool& commandListPool() { return m_commandListPool; }

	LLGL::SwapChain* createSwapChain(const uint16_t width, const uint16_t height, std::string_view debugName = "swapChain");

private:
	bool isAnyWindowOpen();

private:
	using DebuggerPtr = std::unique_ptr<LLGL::RenderingDebugger>;
	using SwapChains = std::vector<LLGL::SwapChain*>;

	CommandListPool m_commandListPool;
	SwapChains m_swapChains;
	LLGL::RenderSystemPtr m_renderer;
	DebuggerPtr m_debugger;
	GraphicsAPI m_gapi = GraphicsAPI::Unknown;
};

} //-- engine.
