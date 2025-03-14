#include <engine/services/world_service.h>
#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/render_service.h>
#include <engine/services/windows_service.h>

#include <imgui.h>
#include <string_view>

using namespace std::string_view_literals;
using namespace entt::literals;

namespace engine
{

namespace
{
/*reflectionMetaInitialization = []() -> void
{
	entt::meta_factory<WorldService>{}.type("WorldService"_hs);
} ();*/

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

	auto& rs = service<RenderService>();
	auto* commandList = rs.commandListPool().requestCommandList();
	auto& swapChain = *service<WindowsService>().mainWindow()->swapChain;
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
