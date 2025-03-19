#pragma once

#include <engine/resources/resource.h>

namespace engine::resources
{

class ShaderResource : public IResource
{
public:
	struct Impl;

	using Shader = std::pair<const void*, size_t>;

	enum class Type : uint8_t
	{
		Vertex,
		Pixel,
		Compute,
		Amplification,
		Mesh,
		Count
	};

	[[nodiscard]] virtual Shader shader(const Type type) = 0;

	//-- Releases internal memory. You may call it after using this data.
	virtual void release() = 0;
};

using ShaderResourcePtr = std::shared_ptr<ShaderResource>;

} //-- engine::resources.
