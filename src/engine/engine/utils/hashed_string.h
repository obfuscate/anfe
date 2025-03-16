#pragma once

namespace engine
{

namespace utils
{

//-- magic numbers from http://www.isthe.com/chongo/tech/comp/fnv/
inline constexpr std::uint32_t fnv1a_32(const char *s, std::size_t count) {
	return count ? (fnv1a_32(s, count - 1) ^ s[count - 1]) * 16777619u : 2166136261u;
}

} //-- utils.

[[nodiscard]] inline constexpr std::uint32_t operator""_hs(const char* string, std::size_t count) noexcept
{
	return utils::fnv1a_32(string, count);
}

} //-- engine.
