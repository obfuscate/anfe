#pragma once
#include <entt/entt.hpp>

template<>
struct std::hash<entt::meta_type>
{
	std::size_t operator()(const entt::meta_type& t) const
	{
		return std::hash<int>{}(t.id());
	}
};

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)

#define META_REGISTRATION \
void auto_register_reflection_function_(); \
namespace \
{ \
	struct auto__register__ \
	{ \
		auto__register__() \
		{ \
			auto_register_reflection_function_(); \
		} \
	}; \
} \
const auto__register__ CONCAT(auto_register__, __LINE__); \
void auto_register_reflection_function_()

namespace integration::entt
{

template<typename Enum>
void enumSetter(Enum& elem, Enum value) {
	elem = value;
}

template<typename Enum>
Enum enumGetter(Enum& elem) {
	return elem;
}

} //-- integration::entt.
