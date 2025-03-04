#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

class WorldService final : public Service<WorldService>
{
public:
	WorldService() = default;
	~WorldService() = default;

	bool initialize() override;
	void release() override;

	void tick() override;

private:
};

} //-- engine.
