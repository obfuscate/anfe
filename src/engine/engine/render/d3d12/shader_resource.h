#pragma once

#include <engine/integration/d3d12/integration.h>
#include <engine/assert.h>
#include <engine/resources/shader_resource.h>

namespace engine::render::d3d12
{

class ShaderResource : public resources::ShaderResource
{
public:
	[[nodiscard]] Shader shader(const Type type) override
	{
		auto& shader = m_shaders[static_cast<uint8_t>(type)];
		ENGINE_ASSERT(shader.Get() != nullptr, "You try to unexisted shader");

		return Shader{ shader->GetBufferPointer(), shader->GetBufferSize() };
	}

	void release() override
	{
		for (auto& blob : m_shaders)
		{
			blob.Reset();
		}
	}

public:
#if 0
	//-- std::array isn't compiled.
	using Shaders = std::array<Microsoft::WRL::ComPtr<IDxcBlob>, static_cast<size_t>(Type::Count)>;
#else
	using Shaders = std::vector<Microsoft::WRL::ComPtr<IDxcBlob>>;
#endif
	Shaders m_shaders;
};

} //-- engine::render::d3d12.
