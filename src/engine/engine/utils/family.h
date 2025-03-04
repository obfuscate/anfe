#pragma once

#include <engine/reflection/common.h>

namespace engine::utils
{

template<typename Type, typename TFamily, typename TClass>
Type familyId()
{
	return static_cast<Type>(familyId(rttr::type::get<TFamily>(), rttr::type::get<TClass>()));
}

ENGINE_API size_t familyId(rttr::type family, rttr::type type);

} //-- engine::utils.
