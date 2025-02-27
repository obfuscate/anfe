#pragma once

namespace engine::utils
{

struct NonCopyable
{
	NonCopyable() = default;

	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;

	//-- support moving.
	NonCopyable(NonCopyable&&) = default;
	NonCopyable& operator= (NonCopyable&&) = default;
};

} //-- engine::utils.
