#pragma once

namespace engine
{

namespace utils
{

//-- magic numbers from http://www.isthe.com/chongo/tech/comp/fnv/
inline constexpr std::uint32_t fnv1a_32(const char *s, std::size_t count) {
	return count ? (fnv1a_32(s, count - 1) ^ s[count - 1]) * 16777619u : 2166136261u;
}

inline std::wstring convertToWideString(const char* string)
{
	std::wstring result;

	std::mbstate_t state = std::mbstate_t();
	std::size_t len = 1 + std::mbsrtowcs(nullptr, &string, 0, &state);
	result.resize(len);
	std::mbsrtowcs(result.data(), &string, result.size(), &state);

	return result;
}

inline std::wstring convertToWideString(const std::string& string)
{
	return convertToWideString(string.c_str());
}

} //-- utils.

[[nodiscard]] inline constexpr std::uint32_t operator""_hs(const char* string, std::size_t count) noexcept
{
	return utils::fnv1a_32(string, count);
}

} //-- engine.
