#include <engine/services/cli_service.h>

namespace engine
{

CLIService::CLIService(const int argc, const char* const argv[])
	: IService()
	, m_parser(argc, argv) { }

bool CLIService::initialize()
{
	//-- ToDO: parse cli.
	return true;
}

} //-- engine.
