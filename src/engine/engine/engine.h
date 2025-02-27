#pragma once
#include <engine/export.h>

namespace engine
{

class ENGINE_API Engine final
{
public:
	Engine() = default;

	bool initialize(const int argc, const char* const argv[]);
	void run();

private:
};

} //-- engine.
