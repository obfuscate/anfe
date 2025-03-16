#pragma once

#include <engine/reflection/common.h>

namespace engine::reflection
{

class BaseService
{
public:
	using CLIArgs = std::vector<std::string_view>;
};

#if USE_ENTT
template<typename T>
class Service : public BaseService, public entt::meta_factory<T>
{
public:
	Service(std::string_view name)
	{
		using namespace entt::literals;

		static_assert(std::is_base_of_v<IService, T>, "Your class has to be inherited from IService!");
		//-- TODO: Add check the passed name: empty, already registered.

		type(operator""_hs(name.data(), name.size()))
			.base<IService>();
	}

	Service& cli(const CLIArgs& /*args*/)
	{
		//this->data()

		return *this;
	}
};


template<typename T>
class Enum : public entt::meta_factory<T>
{
public:
	Enum(std::string_view name)
	{
		using namespace entt::literals;

		static_assert(std::is_enum_v<T>, "Your try to register not enum class!");
		//-- TODO: Add check the passed name: empty, already registered.

		type(operator""_hs(name.data(), name.size()))
			.data<integration::entt::enumSetter<T>, integration::entt::enumGetter<T>>("value"_hs);
	}

	template<typename Value>
	Enum& value(Value value, std::string_view name)
	{
		data<Value>(operator""_hs(name.data(), name.size()));

		return *this;
	}
};

#else
template<typename T>
class Service : public BaseService, public rttr::registration::class_<T>
{
public:
	Service(rttr::string_view name) : class_(name)
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
#endif

} //-- engine::reflection.
