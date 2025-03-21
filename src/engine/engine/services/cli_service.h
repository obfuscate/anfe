#pragma once

#include <engine/engine.h>
#include <engine/services/service_manager.h>
#include <argh/argh.h>

namespace engine
{

class CLIService final : public Service<CLIService>
{
public:
	CLIService();
	~CLIService() = default;

	bool initialize(const Engine::Config::CLIParams& params);

	const argh::parser& parser() const { return m_parser; }

private:
	argh::parser m_parser;
};

} //-- engine.
