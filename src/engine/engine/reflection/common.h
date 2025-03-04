#pragma once

#include <rttr/registration>
#include <fmt/core.h>

template <> struct fmt::formatter<rttr::string_view>
{
	constexpr auto parse(const format_parse_context& ctx) -> decltype(ctx.begin()) { return ctx.begin(); }

	template <typename Context>
	constexpr auto format(rttr::string_view s, Context& ctx) const
	{
		return format_to(ctx.out(), "{}", s.data());
	}
};
