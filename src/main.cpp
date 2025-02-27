#include <engine/engine.h>

int main(int argc, char* argv[])
{
	engine::Engine engine;

	if (!engine.initialize(argc, argv))
	{
		return 0;
	}

	engine.run();

	return 0;
}
