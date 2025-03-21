#include <engine/services/cli_service.h>
#include <engine/reflection/registration.h>

#define OUTPUT_DEBUG_CLI 0

namespace engine
{

namespace
{

META_REGISTRATION
{
	reflection::Service<CLIService>("CLIService");
}

} //-- unnamed.


CLIService::CLIService()
	: Service()
	, m_parser() { }


bool CLIService::initialize(const Engine::Config::CLIParams& params)
{
	auto baseService = rttr::type::get<IService>();
	for (auto child : baseService.get_derived_classes())
	{
		if (auto cliMeta = child.get_metadata(IService::kMetaCLI))
		{
			const auto& cli = cliMeta.get_value<reflection::details::BaseService::CLIArgs>();
			for (auto param : cli)
			{
				m_parser.add_param(std::string(param));
			}
		}
	}

	const int flags = argh::parser::PREFER_FLAG_FOR_UNREG_OPTION;
	m_parser.parse(params.numArguments, params.arguments, flags);

#if OUTPUT_DEBUG_CLI
	for (auto& flag : m_parser.flags())
	{
		fmt::println(flag);
	}
	for (auto& [k, v] : m_parser.params())
	{
		fmt::println("{} -> {}", k, v);
	}
#endif

	return true;
}

} //-- engine.
