#pragma once

#include <engine/engine.h>
#include <engine/services/service_manager.h>
#include <engine/render/render_backend.h>

namespace engine
{

enum class GraphicsAPI : uint8_t
{
	DirectX12 = 0,
	Unknown
};

class RenderService final : public Service<RenderService>
{
public:
	using CommandList = void;

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

	bool initialize(const Engine::Config::RenderParams& params);
	void release() override;

	void postTick() override;

	GraphicsAPI gapi() const { return m_gapi; }

	//-- ToDo: Remove and add create section.
	render::IBackend* backend() { return m_backend.get(); }

private:
	using BackendPtr = std::unique_ptr<render::IBackend>;
	//using SwapChains = std::vector<LLGL::SwapChain*>;

	CommandListPool m_commandListPool;
	BackendPtr m_backend;
	//SwapChains m_swapChains;
	GraphicsAPI m_gapi = GraphicsAPI::Unknown;

};

} //-- engine.
