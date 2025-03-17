#pragma once

#include <engine/reflection/common.h>

namespace engine::reflection
{

namespace details
{

class BaseService
{
public:
	using CLIArgs = std::vector<std::string_view>;
};

} //-- details.


template<typename T>
class Service : public details::BaseService, public rttr::registration::class_<T>
{
public:
	Service(rttr::string_view name) : BaseService(), rttr::registration::class_<T>(name)
	{
		static_assert(std::is_base_of_v<IService, T>, "Your class has to be inherited from IService!");
		//-- TODO: Add check the passed name: empty, already registered.
	}

	Service& cli(const CLIArgs& args)
	{
		this->operator()
		(
			rttr::metadata(IService::kMetaCLI, args)
		);

		return *this;
	}
};

} //-- engine::reflection.
