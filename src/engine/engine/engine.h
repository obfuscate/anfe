#pragma once
#include <engine/export.h>

namespace engine
{

class ENGINE_API Engine final
{
public:
	Engine() = default;

	bool initialize();
	void run();

private:
};

} //-- engine.
