#include <engine/services/editor_service.h>
#include <engine/reflection/registration.h>

#include <imgui.h>

namespace engine
{

namespace
{

META_REGISTRATION
{
	reflection::Service<EditorService>("EditorService");
}

} //-- unnamed.

void EditorService::tick()
{
	//ImGui::ShowDemoWindow();
}

} //-- engine.
