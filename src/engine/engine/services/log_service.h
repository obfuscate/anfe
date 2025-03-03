#pragma once

#include <engine/export.h>
#include <engine/pch.h>

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

private:
	std::shared_ptr<spdlog::logger> m_logger;
};

} //-- engine.
