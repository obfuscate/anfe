#pragma once

#include <engine/integration/math/integration.h>

namespace engine::math
{

using vec2 = integration::math::vec2;
using vec3 = integration::math::vec3;
using vec4 = integration::math::vec4;

using ivec2 = integration::math::ivec2;
using ivec3 = integration::math::ivec3;
using ivec4 = integration::math::ivec4;

using uvec2 = integration::math::uvec2;
using uvec3 = integration::math::uvec3;
using uvec4 = integration::math::uvec4;

using matrix = integration::math::matrix;

using color = integration::math::color;

using quat = integration::math::quat;

constexpr float kPi = DirectX::XM_PI;
constexpr float k2Pi = DirectX::XM_2PI;

FORCE_INLINE math::vec3 min(const vec3& lhs, const vec3& rhs) { return integration::math::min(lhs, rhs); }
FORCE_INLINE math::vec3 max(const vec3& lhs, const vec3& rhs) { return integration::math::max(lhs, rhs); }


} //-- engine::math.
