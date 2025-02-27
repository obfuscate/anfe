#include <engine/engine.h>

#include <ufbx/ufbx.h>

#include <iostream>

namespace engine
{

bool Engine::initialize()
{
	ufbx_load_opts opts = { 0 };
	ufbx_error error;
	[[maybe_unused]] ufbx_scene* scene = ufbx_load_file("thing.fbx", &opts, &error);

	std::cout << "Initialize" << std::endl;
	return true;
}

void Engine::run()
{
	std::cout << "Run!" << std::endl;
}

} //-- engine.
