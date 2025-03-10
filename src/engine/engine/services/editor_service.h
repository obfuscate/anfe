#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

class EditorService final : public Service<EditorService>
{
public:
	EditorService() = default;
	~EditorService() = default;

	bool initialize() override { return true; }
	void release() override {};

	void tick() override;
};

} //-- engine.
