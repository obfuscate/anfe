#pragma once

#include <engine/reflection/common.h>

namespace engine::utils
{

#if USE_ENTT
template<typename Type, typename Family, typename Class>
Type familyId()
{
	return static_cast<Type>(familyId(entt::resolve<Family>(), entt::resolve<Class>()));
}

ENGINE_API size_t familyId(entt::meta_type family, entt::meta_type type);
#else
template<typename Type, typename Family, typename Class>
Type familyId()
{
	return static_cast<Type>(familyId(rttr::type::get<Family>(), rttr::type::get<Class>()));
}

ENGINE_API size_t familyId(rttr::type family, rttr::type type);
#endif


} //-- engine::utils.
