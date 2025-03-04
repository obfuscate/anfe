#pragma once

#include <engine/services/service_manager.h>

namespace engine
{

//-- Singleton of the Engine.
class Engine final : public utils::NonCopyable
{
public:
	ENGINE_API static Engine& instance();

	ENGINE_API bool initialize(const int argc, const char* const argv[]);
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
inline Engine& instance()
{
	return Engine::instance();
}

} //-- engine.
