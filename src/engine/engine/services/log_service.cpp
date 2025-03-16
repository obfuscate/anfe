#include <engine/services/log_service.h>
#include <engine/engine.h>
#include <engine/helpers.h>
#include <engine/services/cli_service.h>
#include <engine/reflection/registration.h>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace engine
{


namespace
{

META_REGISTRATION
{
	reflection::Service<LogService>("LogService")
		.cli({"--logfile", "-logstd"})
	;
}


inline constexpr std::string_view kPattern = "[%H:%M:%S:%e] [thread %5t] %^[%L] %v%$";

} //-- unnamed.


bool LogService::initialize()
{
	auto& cli = service<CLIService>().parser();

	spdlog::filename_t filename;
	cli("--logfile", "logs/log.txt") >> filename;

	//-- ToDo: Use another logger, e.g. rotating or asynchronous. See https://github.com/gabime/spdlog/wiki/1.-QuickStart for more details.
	m_logger = spdlog::basic_logger_mt("fileLogger", filename, true);
	ENGINE_ASSERT(m_logger != nullptr, "Couldn't create a logger!");

	auto pattern = std::string(kPattern);
	auto timePattern = spdlog::pattern_time_type::local;
	m_logger->set_pattern(pattern, timePattern);
	spdlog::set_default_logger(m_logger);

	if (cli["-logstd"])
	{
		m_stdLogger = spdlog::stdout_color_mt("consoleLogger");
		ENGINE_ASSERT(m_logger != nullptr, "Couldn't create a logger!");

		m_stdLogger->set_pattern(pattern, timePattern);
	}

	info("Logger is initialized");

	return true;
}


void LogService::release()
{
	flush();

	m_logger.reset();
	m_stdLogger.reset();
}


void LogService::tick()
{
	flush();
}


void LogService::flush()
{
	m_logger->flush();
	m_stdLogger->flush();
}

} //-- engine.
