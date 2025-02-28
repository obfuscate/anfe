#pragma once
#include <engine/export.h>
#include <engine/services/service_manager.h>

#include <argh/argh.h>

namespace engine
{

class CLIService final : public Service<CLIService>
{
public:
	CLIService(const int argc, const char* const argv[]);
	~CLIService() = default;

	bool initialize() override;

	const argh::parser& parser() const { return m_parser; }

private:
	argh::parser m_parser;
	const char* const* m_argv = nullptr;
	const int m_argc = 0;
};

} //-- engine.
