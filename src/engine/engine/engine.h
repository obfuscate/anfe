#pragma once

#include <engine/utils/noncopyable.h>
#include <engine/services/service_manager.h>

namespace engine
{

//-- Singleton of the Engine.
class Engine final : public utils::NonCopyable
{
public:
	struct Config
	{
		struct CLIParams
		{
			char** arguments = nullptr;
			uint16_t numArguments = 0;
		};

		struct VFSParams
		{
			struct Alias
			{
				enum class Type : uint8_t
				{
					Native,
					BuildZip
				};

				std::string_view alias;
				std::string_view relativePath;
				Type type = Type::Native;
			};

			std::vector<Alias> aliases;
		};

		struct RenderParams
		{
			uint8_t numBackBuffers = 2;
		};

		VFSParams vfsParams;
		CLIParams cliParams;
		RenderParams renderParams;
	};

	ENGINE_API static Engine& instance();

	ENGINE_API bool initialize(const Config& config);
	ENGINE_API void run();
	ENGINE_API void stop() { m_run = false; }

	ENGINE_API ServiceManager& serviceManager() { return m_serviceManager; }

private:
	Engine() = default;

	void release();

private:
	ServiceManager m_serviceManager;
	bool m_run = false;
};

//-- Helper to avoid redundant Engine::
inline Engine& engine()
{
	return Engine::instance();
}

} //-- engine.
