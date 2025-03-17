#pragma once

#include <engine/services/service_manager.h>

#include <imgui.h> //-- ToDo: Fix the installing imgui.

namespace engine
{

class ImGUIService final : public Service<ImGUIService>
{
public:
	//-- Adopted copy-paste from https://github.com/LukasBanana/LLGL-Example-ImGui
	class Backend
	{
	public:
		struct WindowContext
		{
			//LLGL::SwapChain* swapChain = nullptr;
			ImGuiContext* imGuiContext = nullptr;
			//LLGL::Offset2D mousePosInWindow;
		};

	public:
		virtual ~Backend() = default;

		void initialize();
		void release();

		virtual void beginFrame(/*LLGL::CommandBuffer* commandList*/);
		virtual void endFrame(/*LLGL::CommandBuffer* commandList*/) {}

	protected:
		void createResources();
		virtual void initializeContext(WindowContext& ctx);
		virtual void releaseContext(WindowContext& ctx);

	protected:
		std::vector<WindowContext> m_windowContexts;
	};

public:
	ImGUIService() = default;
	~ImGUIService() = default;

	bool initialize() override;
	void release() override;

	void tick() override;
	void postTick() override;

	Backend* backend() { return m_backend.get(); }

private:
	std::unique_ptr<Backend> m_backend;
	//LLGL::CommandBuffer* m_commandList = nullptr;
};

} //-- engine.
