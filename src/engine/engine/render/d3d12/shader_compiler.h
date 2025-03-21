#pragma once

#include <engine/integration/d3d12/integration.h>
#include <engine/render/shader_compiler.h>
#include <engine/render/d3d12/shader_resource.h>

namespace engine::render::d3d12
{

//-- This is instance should be per-thread to achieve multithreading safe.
class ShaderCompiler : public IShaderCompiler
{
public:
	~ShaderCompiler() = default;

	ENGINE_API bool initialize() override;

	ENGINE_API resources::ShaderResourcePtr compile(std::string_view path) override;

private:
	struct Blob
	{
		void initialize(const size_t size)
		{
			m_data.resize(size);
		}

		const uint8_t* data() const { return m_data.data(); }
		size_t size() const { return m_data.size(); }

		std::vector<uint8_t> m_data;
	};

	bool compile(const Blob& blob, const std::string& absolutePath, resources::ShaderResource::Type type, ShaderResource& resource);

private:
	std::vector<LPCWSTR> m_commonArguments;
	std::vector<LPCWSTR> m_shaderArguments;
	Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_includeHandler;
	Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
};

} //-- engine::render::d3d12.
