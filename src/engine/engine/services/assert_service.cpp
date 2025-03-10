#include <engine/services/assert_service.h>
#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/log_service.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	reflection::Service<AssertService>("AssertService");
}


void handler(const libassert::assertion_info& info)
{
	auto logService = findService<LogService>();
	if (logService && info.message)
	{
		logService->critical(info.message.value());
		logService->flush();
	}
	libassert::default_failure_handler(info);
}

} //-- unnamed.


bool AssertService::initialize()
{
	libassert::set_failure_handler(handler);

	return true;
}


void AssertService::release()
{

}

} //-- engine.
