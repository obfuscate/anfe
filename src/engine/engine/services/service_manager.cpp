#include <engine/services/service_manager.h>

namespace engine
{

void ServiceManager::release()
{
	for (auto& service : m_services)
	{
		service->release();
	}
}

} //-- engine.
