#include <engine/services/service_manager.h>

namespace engine
{

void ServiceManager::tick()
{
	for (auto& service : m_services)
	{
		service->tick();
	}
}


void ServiceManager::release()
{
	for (auto it = m_services.rbegin(); it != m_services.rend(); ++it)
	{
		it->release();
	}
}

} //-- engine.
