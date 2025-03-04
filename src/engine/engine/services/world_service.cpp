#include <engine/services/world_service.h>
#include <engine/engine.h>
#include <engine/reflection/registration.h>
#include <engine/services/render_service.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	reflection::Service<WorldService>("WorldService");
}

} //-- unnamed.


bool WorldService::initialize()
{
	return true;
}


void WorldService::release()
{

}


void WorldService::tick()
{
	auto& rs = instance().serviceManager().get<RenderService>();
	auto* commandList = rs.commandListPool().requestCommandList();
	auto& swapChain = *rs.swapChain();
	commandList->Begin(); //-- ToDo: move to requestCommandList().
	{
		// Bind common input assembly
		//commands->SetVertexBuffer(*vertexBuffer);

		// Render everything directly into the swap-chain
		commandList->BeginRenderPass(swapChain);
		{
			commandList->Clear(LLGL::ClearFlags::ColorDepth, LLGL::ClearValue({0.2f, 0.8f, 0.4f, 1.0f}));
			commandList->SetViewport(swapChain.GetResolution());
			//RenderScene();
		}
		commandList->EndRenderPass();
	}
	commandList->End(); //-- ToDo: Move to submitCommandLists();
}

} //-- engine.
