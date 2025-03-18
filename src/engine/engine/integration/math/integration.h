#pragma once

#include <engine/integration/math/SimpleMath.h>

//-- Just copy-paste of VectorX and add an opportunity to use it as int/uint vectors.
namespace DirectX
{

namespace SimpleMath
{

#include <engine/integration/math/ivec2.h>
#include <engine/integration/math/ivec3.h>
#include <engine/integration/math/ivec4.h>
#include <engine/integration/math/uvec2.h>
#include <engine/integration/math/uvec3.h>
#include <engine/integration/math/uvec4.h>
} //-- SimpleMath.

} //-- DirectX.


//-- We may place here all wrappers for 3rdparty math lib to easily switch them in the future.
//-- But I'm too lazy right now to do it.
namespace engine::integration::math
{
using vec2 = DirectX::SimpleMath::Vector2;
using vec3 = DirectX::SimpleMath::Vector3;
using vec4 = DirectX::SimpleMath::Vector4;

using ivec2 = DirectX::SimpleMath::IVector2;
using ivec3 = DirectX::SimpleMath::IVector3;
using ivec4 = DirectX::SimpleMath::IVector2;

using uvec2 = DirectX::SimpleMath::UVector2;
using uvec3 = DirectX::SimpleMath::UVector2;
using uvec4 = DirectX::SimpleMath::UVector2;

using matrix = DirectX::SimpleMath::Matrix;

using color = DirectX::SimpleMath::Color;

using quat = DirectX::SimpleMath::Quaternion;
}
