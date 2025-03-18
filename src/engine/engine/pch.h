#pragma once

#include <assert.h>
#include <concepts>
#include <intrin.h>
#include <filesystem>
#include <memory>
#include <mutex>
#include <vector>
#include <string>
#include <string_view>
#include <type_traits>

//-- fmt.
#include <fmt/core.h>

//-- SDL3
#include <SDL3/SDL.h>

#include <engine/export.h>

#if defined(__clang__)
#define FORCE_INLINE [[gnu::always_inline]] [[gnu::gnu_inline]] extern inline

#elif defined(__GNUC__)
#define FORCE_INLINE [[gnu::always_inline]] inline

#elif defined(_MSC_VER)
#pragma warning(error: 4714)
#define FORCE_INLINE __forceinline

#else
#error Unsupported compiler
#endif
