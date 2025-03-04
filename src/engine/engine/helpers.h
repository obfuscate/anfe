#pragma once
#include <engine/engine.h>
#include <engine/services/log_service.h>

namespace engine
{

inline LogService& logger()
{
	return instance().serviceManager().get<LogService>();
}

} //-- engine::helpers.
