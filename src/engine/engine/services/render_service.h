#pragma once

#include <engine/services/service_manager.h>
#include <engine/render/render_backend.h>

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
	using CommandList = void;// LLGL::CommandBuffer;

	class CommandListPool
	{
	public:
		CommandList* requestCommandList();
		//-- ToDo: Remove later.
		CommandList* imguiCommandList() { return nullptr; }

	private:
		void initialize();
		void submit();

		friend class RenderService;

	private:
	};

public:
	RenderService() = default;
	~RenderService() = default;

	bool initialize() override;
	void release() override;

	void postTick() override;

	GraphicsAPI gapi() const { return m_gapi; }

private:
	using BackendPtr = std::unique_ptr<render::IBackend>;
	//using SwapChains = std::vector<LLGL::SwapChain*>;

	CommandListPool m_commandListPool;
	BackendPtr m_backend;
	//SwapChains m_swapChains;
	GraphicsAPI m_gapi = GraphicsAPI::Unknown;

};

} //-- engine.
