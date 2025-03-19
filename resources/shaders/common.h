#pragma once

#ifndef SHADERS
using float2 = engine::math::vec2;
using float3 = engine::math::vec3;
using float4 = engine::math::vec4;

using int2 = engine::math::ivec2;
using int3 = engine::math::ivec3;
using int4 = engine::math::ivec4;

using uint2 = engine::math::uvec2;
using uint3 = engine::math::uvec3;
using uint4 = engine::math::uvec4;

using float4x4 = engine::math::matrix;
#endif

cbuffer Global : register(b0)
{
	float4 g_offset;
};
