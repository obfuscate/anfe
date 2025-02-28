#pragma once

#include <engine/export.h>
#include <engine/pch.h>
#include <engine/reflection/common.h>

namespace engine::reflection
{
//-- Tips:
//-- 1. When you use class hierarchies you should add to every class : RTTR_ENABLE(base_classes_list).
//-- 2. When you want to reflect private data of a class, add : RTTR_REGISTRATION_FRIEND.
class BaseService
{
public:
	using CLIArgs = std::vector<std::string_view>;
};

template<typename TService>
class Service : public rttr::registration::class_<TService>, public BaseService
{
public:
	ENGINE_API Service(rttr::string_view name) : class_(name)
	{
		static_assert(std::is_base_of_v<IService, TService>, "Your class has to be inherited from IService!");
	}

	ENGINE_API Service& cli(const CLIArgs& args)
	{
		this->operator()
			(
				rttr::metadata(42, args)
			);

		return *this;
	}
private:

};

} //-- engine::reflection.
