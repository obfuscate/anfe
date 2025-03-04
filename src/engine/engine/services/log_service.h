#pragma once

#include <engine/services/service_manager.h>

#include <spdlog/spdlog.h>

namespace engine
{

class LogService final : public Service<LogService>
{
public:
	LogService() = default;
	~LogService() = default;

	bool initialize() override;
	void release() override;

	void tick() override;

	void flush();

	template<typename T>
	void log(spdlog::level::level_enum level, const T& message)
	{
		m_logger->log(level, message);
		m_stdLogger->log(level, message);
	}

	template<typename T>
	void trace(const T& message)
	{
		log(spdlog::level::trace, message);
	}

	template<typename T>
	void debug(const T& message)
	{
		log(spdlog::level::debug, message);
	}

	template<typename T>
	void info(const T& message)
	{
		log(spdlog::level::info, message);
	}

	template<typename T>
	void warning(const T& message)
	{
		log(spdlog::level::warn, message);
	}

	template<typename T>
	void error(const T& message)
	{
		log(spdlog::level::err, message);
	}

	template<typename T>
	void critical(const T& message)
	{
		log(spdlog::level::critical, message);
	}

private:
	std::shared_ptr<spdlog::logger> m_logger;
	std::shared_ptr<spdlog::logger> m_stdLogger;
};

} //-- engine.
