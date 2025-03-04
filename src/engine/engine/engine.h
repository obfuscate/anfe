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

	ENGINE_API ServiceManager& serviceManager() { return m_serviceManager; }

private:
	Engine() = default;
	~Engine();

	void release();
	void mainLoop();
	void drawFrame();

private:
	ServiceManager m_serviceManager;
	LLGL::Input m_input; //-- ToDo: Do move to input_service?
};

//-- Helper to avoid redundant Engine::
inline Engine& instance()
{
	return Engine::instance();
}

} //-- engine.
