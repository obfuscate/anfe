#include <engine/services/log_service.h>
#include <engine/services/cli_service.h>
#include <engine/reflection/registration.h>

#include <spdlog/sinks/basic_file_sink.h>

namespace engine
{


namespace
{

RTTR_REGISTRATION
{
	reflection::Service<LogService>("LogService")
		.cli({"--file"})
	;
}

} //-- unnamed.


bool LogService::initialize()
{
	auto& cli = instance().serviceManager().get<CLIService>().parser();

	spdlog::filename_t filename;
	cli("--file", "logs/log.txt") >> filename;

	//-- ToDo: Use another logger, e.g. rotating or asynchronous. See https://github.com/gabime/spdlog/wiki/1.-QuickStart for more details.
	m_logger = spdlog::basic_logger_mt("logger", filename, true);
	ENGINE_ASSERT(m_logger != nullptr, "Couldn't create a logger!");
	m_logger->info("Logger is initialized");

	spdlog::set_default_logger(m_logger);

	return true;
}


void LogService::release()
{
	m_logger.reset();
}

} //-- engine.
