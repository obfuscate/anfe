#pragma once
#include <engine/engine.h>
#include <engine/services/log_service.h>

namespace engine
{

inline spdlog::logger& logger()
{
	return *spdlog::default_logger();
}

} //-- engine::helpers.
