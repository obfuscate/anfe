#pragma once
#include <engine/export.h>
#include <engine/services/service.h>

#include <argh/argh.h>

namespace engine
{

class CLIService final : public IService
{
public:
	CLIService(const int argc, const char* const argv[]);
	~CLIService() = default;

	bool initialize() override;

private:
	argh::parser m_parser;
};

} //-- engine.
