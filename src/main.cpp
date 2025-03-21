#include <engine/engine.h>

int main(int argc, char* argv[])
{
	auto& engine = engine::engine();

	engine::Engine::Config config;

	//-- We take the path to the root folder from the CLI.
	config.vfsParams.aliases = { { "/", "/resources", engine::Engine::Config::VFSParams::Alias::Type::Native }};
	config.cliParams = { .arguments = static_cast<char**>(argv), .numArguments = static_cast<uint16_t>(argc) };
	config.renderParams = { .numBackBuffers = 2 };

	if (!engine.initialize(config))
	{
		return 0;
	}

	engine.run();

	return 0;
}
