#pragma once

#include <engine/resources/shader_resource.h>

namespace engine::render
{

class IShaderCompiler
{
public:
	ENGINE_API virtual ~IShaderCompiler() = default;

	ENGINE_API virtual bool initialize() = 0;
	ENGINE_API virtual resources::ShaderResourcePtr compile(std::string_view path) = 0;
};

} //-- engine::render.
