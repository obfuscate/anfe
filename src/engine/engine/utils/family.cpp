#include <engine/utils/family.h>

namespace engine::utils
{

size_t familyId(rttr::type family, rttr::type type)
{
	static std::unordered_map<rttr::type, std::unordered_map<rttr::type, size_t>> s_familyIds;
	static std::mutex s_mutex;

	std::lock_guard g(s_mutex);
	auto& types = s_familyIds[family];
	auto result = types.try_emplace(type, types.size());
	return result.first->second;
}

} //-- engine::utils.
