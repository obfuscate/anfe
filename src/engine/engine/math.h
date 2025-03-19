#pragma once

#include <engine/integration/math/integration.h>

namespace engine::math
{

using vec2 = engine::integration::math::vec2;
using vec3 = engine::integration::math::vec3;
using vec4 = engine::integration::math::vec4;

using ivec2 = engine::integration::math::ivec2;
using ivec3 = engine::integration::math::ivec3;
using ivec4 = engine::integration::math::ivec4;

using uvec2 = engine::integration::math::uvec2;
using uvec3 = engine::integration::math::uvec3;
using uvec4 = engine::integration::math::uvec4;

using matrix = engine::integration::math::matrix;

using color = engine::integration::math::color;

using quat = engine::integration::math::quat;

constexpr float kPi = DirectX::XM_PI;
constexpr float k2Pi = DirectX::XM_2PI;


} //-- engine::math.
