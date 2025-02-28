#include <engine/engine.h>

int main(int argc, char* argv[])
{
	auto& engine = engine::instance();

	if (!engine.initialize(argc, argv))
	{
		return 0;
	}

	engine.run();

	return 0;
}
