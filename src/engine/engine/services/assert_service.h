#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

class AssertService final : public Service<AssertService>
{
public:
	AssertService() = default;
	~AssertService() = default;

	bool initialize() override;
	void release() override;

private:

};

} //-- engine.
