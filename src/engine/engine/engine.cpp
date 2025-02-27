#include <engine/engine.h>

#include <argh/argh.h>
#include <fmt/core.h>

namespace engine
{

bool Engine::initialize(const int argc, const char* const argv[])
{
	fmt::println("Initialize!");
	return true;
}

void Engine::run()
{
	fmt::println("Run!");
}

} //-- engine.
