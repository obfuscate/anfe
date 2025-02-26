#include <engine/engine.h>

int main()
{
	engine::Engine engine;

	if (!engine.initialize())
	{
		return 0;
	}

	engine.run();

	return 0;
}
