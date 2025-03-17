#pragma once

#include <engine/reflection/common.h>

namespace engine::utils
{

ENGINE_API size_t familyId(rttr::type family, rttr::type type);

template<typename Type, typename Family, typename Class>
Type familyId()
{
	return static_cast<Type>(familyId(rttr::type::get<Family>(), rttr::type::get<Class>()));
}


} //-- engine::utils.
