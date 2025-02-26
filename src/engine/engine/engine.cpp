#include <engine/engine.h>

#include <iostream>

namespace engine
{

bool Engine::initialize()
{
	std::cout << "Initialize" << std::endl;
	return true;
}

void Engine::run()
{
	std::cout << "Run!" << std::endl;
}

} //-- engine.
