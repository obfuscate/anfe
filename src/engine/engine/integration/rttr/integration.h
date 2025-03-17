#pragma once

#include <rttr/registration>

#ifdef RTTR_ENABLE
#define META_REGISTRATION RTTR_REGISTRATION
#else
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
#endif
