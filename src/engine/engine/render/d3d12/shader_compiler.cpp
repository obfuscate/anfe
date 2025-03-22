#include <engine/render/d3d12/shader_compiler.h>
#include <engine/assert.h>
#include <engine/helpers.h>
#include <engine/render/d3d12/shader_resource.h>
#include <engine/services/vfs_service.h>
#include <engine/utils/string.h>

namespace engine::render::d3d12
{

namespace
{

//-- https://simoncoenen.com/blog/programming/graphics/DxcCompiling
//-- This is useful if you want to add some custom include logic or if you want to know
//-- which files were included to do some kind of shader hot-reloading.
//-- Here's an example implementation that correctly loads include sources and
//-- makes sure not to include the same file twice.
//-- To add include directories, use the -I argument like for example -I /Resources/Shaders.
/*class CustomIncludeHandler : public IDxcIncludeHandler
{
public:
	HRESULT STDMETHODCALLTYPE LoadSource(_In_ LPCWSTR pFilename, _COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource) override
	{
		ComPtr<IDxcBlobEncoding> pEncoding;
		std::string path = Paths::Normalize(UNICODE_TO_MULTIBYTE(pFilename));
		if (IncludedFiles.find(path) != IncludedFiles.end())
		{
			// Return empty string blob if this file has been included before
			static const char nullStr[] = " ";
			pUtils->CreateBlobFromPinned(nullStr, ARRAYSIZE(nullStr), DXC_CP_ACP, pEncoding.GetAddressOf());
			*ppIncludeSource = pEncoding.Detach();
			return S_OK;
		}

		HRESULT hr = pUtils->LoadFile(pFilename, nullptr, pEncoding.GetAddressOf());
		if (SUCCEEDED(hr))
		{
			IncludedFiles.insert(path);
			*ppIncludeSource = pEncoding.Detach();
		}
		return hr;
	}

	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override { return E_NOINTERFACE; }
	ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
	ULONG STDMETHODCALLTYPE Release(void) override { return 0; }

	std::unordered_set<std::string> IncludedFiles;
};*/

} //-- unnamed.


using Microsoft::WRL::ComPtr;

bool ShaderCompiler::initialize()
{
	HRESULT ok = S_OK;
	ok = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(m_utils.ReleaseAndGetAddressOf()));
	ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't create an instance of dxc utils.");

	ok = m_utils->CreateDefaultIncludeHandler(m_includeHandler.ReleaseAndGetAddressOf());
	ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't create an instance of dxc include handler.");

	ok = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler));
	ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't create an instance of dxc compiler.");

	//-- default arguments for compiler.
	{
		//-- -Fc <file>
		//-- -Fe <file>              Output warnings and errors to the given file
		//-- -Fh <file>              Output header file containing object code
		//-- -Fi <file>              Set preprocess output file name(with / P)
		//-- -Fsh <file>             Output shader hash to the given file
		//-- -Gfa                    Avoid flow control constructs
		//-- -Gfp                    Prefer flow control constructs
		//-- -Odump                  Print the optimizer commands.
		//-- -decl-global-cb         Collect all global constants outside cbuffer declarations into cbuffer GlobalCB { ... }. Still experimental, not all dependency scenarios handled.
		//-- -extract-entry-uniforms Move uniform parameters from entry point to global scope

		m_commonArguments.push_back(DXC_ARG_ENABLE_STRICTNESS); //-- Enable strict mode.
		m_commonArguments.push_back(DXC_ARG_IEEE_STRICTNESS); //-- Force IEEE strictness.
		m_commonArguments.push_back(DXC_ARG_WARNINGS_ARE_ERRORS);
		m_commonArguments.push_back(DXC_ARG_PACK_MATRIX_ROW_MAJOR);
		m_commonArguments.push_back(L"-enable-16bit-types"); //-- Enable 16bit types and disable min precision types. Available in HLSL 2018 and shader model 6.2.
		//m_arguments.push_back(L"-pack-optimized"); //-- Optimize signature packing assuming identical signature provided for each connecting stage.
		//m_commonArguments.push_back(L"-remove-unused-functions"); //-- Remove unused functions and types. Errors: Unknown argument: '-remove-unused-functions'
		//m_commonArguments.push_back(L"-remove-unused-globals"); //--  Remove unused static globals and functions. Errors: Unknown argument: '-remove-unused-globals'

		//-- Macroes.
		m_commonArguments.push_back(L"-D");
		m_commonArguments.push_back(L"SHADERS");

		//-- Check disabling optimizations.
		if (false)
		{
			m_commonArguments.push_back(L"-Od");
		}
		//-- Check a specific level of optimizations.
		if (false)
		{
			m_commonArguments.push_back(L"-O0");
		}
	}

	return SUCCEEDED(ok);
}


resources::ShaderResourcePtr ShaderCompiler::compile(std::string_view path)
{
	using ShaderType = resources::ShaderResource::Type;

	auto resource = std::make_shared<ShaderResource>();
	resource->m_shaders.resize(static_cast<size_t>(ShaderType::Count));

	logger().info(fmt::format("[ShaderCompiler]: load the shader '{}'", path));

	Blob blob;
	auto& vfs = service<VFSService>();
	std::bitset<static_cast<size_t>(ShaderType::Count)> enabledShaders;
	//-- Preparse the file, read existed entry points and compile only them.
	{
		if (auto file = vfs.openFile(path))
		{
			blob.initialize(file->Size());
			file->Read(blob.m_data, blob.size());
		}
		else
		{
			logger().error(fmt::format("[ShaderCompiler]: can't open the file '{}'", path));
			return resource;
		}

		std::string_view shader(reinterpret_cast<const char*>(blob.m_data.data()), blob.size());
		size_t offset = 0;
		while (true)
		{
			size_t pos = shader.find("_main", offset);
			if (pos == shader.npos)
			{
				break;
			}

			//-- ToDo: Reconsider later.
			if (shader.compare(pos - 2, 2, "vs"))
			{
				enabledShaders.set(static_cast<uint8_t>(ShaderType::Vertex), true);
			}
			else if (shader.compare(pos - 2, 2, "ps"))
			{
				enabledShaders.set(static_cast<uint8_t>(ShaderType::Pixel), true);
			}
			else if (shader.compare(pos - 2, 2, "cs"))
			{
				enabledShaders.set(static_cast<uint8_t>(ShaderType::Compute), true);
			}
			else if (shader.compare(pos - 2, 2, "as"))
			{
				enabledShaders.set(static_cast<uint8_t>(ShaderType::Amplification), true);
			}
			else if (shader.compare(pos - 2, 2, "ms"))
			{
				enabledShaders.set(static_cast<uint8_t>(ShaderType::Mesh), true);
			}
			offset = pos + 1;
		}

		ENGINE_ASSERT(enabledShaders.any(), "Shader doesn't include any entry points (vs_main, ps_main, cs_main, ms_main)");
	}
	std::string absolutePath = vfs.absolutePath(path);

	bool compiled = true;
	for (uint8_t i = 0; i < static_cast<uint8_t>(ShaderType::Count); ++i)
	{
		if (enabledShaders.test(i))
		{
			compiled &= compile(blob, absolutePath, static_cast<ShaderType>(i), *resource);
		}
	}

	if (compiled)
	{
		resource->setStatus(resources::IResource::Status::Ready);
	}

	return resource;
}

bool ShaderCompiler::compile(const Blob& blob, const std::string& absolutePath, resources::ShaderResource::Type type, ShaderResource& resource)
{
	static constexpr std::array<LPCWSTR, static_cast<uint8_t>(resources::ShaderResource::Type::Count)> kPostfixes = { L".vs", L".ps", L".cs", L".as", L".ms"};
	static constexpr std::array<LPCWSTR, static_cast<uint8_t>(resources::ShaderResource::Type::Count)> kEntryPoints = { L"vs_main", L"ps_main", L"cs_main", L"as_main", L"ms_main"};
	//-- Agility SDK 615 supports 6_8. Without it we have to switch to 6_5.
	static constexpr std::array<LPCWSTR, static_cast<uint8_t>(resources::ShaderResource::Type::Count)> kTargets = { L"vs_6_8", L"ps_6_8", L"cs_6_8", L"as_6_8", L"ms_6_8"};

	std::wstring wPath = utils::convertToWideString(absolutePath.data());

	std::wstring wShaderFolder = utils::convertToWideString(service<VFSService>().absolutePath("/shaders"));
	std::wstring postfixesPath = wPath + kPostfixes[static_cast<uint8_t>(type)];
	std::wstring pdbPath = postfixesPath + L".pdb";
	std::wstring reflectionPath = postfixesPath + L".rfl";
	std::wstring rootSignaturePath = postfixesPath + L".rs";
	std::wstring objectPath = postfixesPath + L".dxo";

	//-- Setup additional params.
	{

		m_shaderArguments = m_commonArguments;

		m_shaderArguments.push_back(L"-I");
		m_shaderArguments.push_back(wShaderFolder.data()); //-- ToDo: Reconsider later.

		//-- Strip all info.
		//-- Debug.
		{
			m_shaderArguments.push_back(L"-Qstrip_debug");
		}
		//-- Private section.
		{
			m_shaderArguments.push_back(L"-Qstrip_priv");
		}
		//-- Root signature.
		{
			m_shaderArguments.push_back(L"-Qstrip_rootsignature");
			m_shaderArguments.push_back(L"-Frs");
			m_shaderArguments.push_back(rootSignaturePath.data());
		}

		//-- Generate symbols.
		if (true) //-- Add an opportunity to disable/enable these options.
		{
			m_shaderArguments.push_back(DXC_ARG_DEBUG);				//-- Enable debug information. Cannot be used together with -Zs.
			//-- -Zs: Generate small PDB with just sources and compile options. Cannot be used together with -Zi
			//m_shaderArguments.push_back(L"-Qembed_debug");	//-- Embed PDB in shader container (must be used with /Zi).
			m_shaderArguments.push_back(L"-Fd");				//-- Write debug information to the given file, or automatically named file in directory when ending in '\'.
			m_shaderArguments.push_back(pdbPath.data());
		}

		//-- Generate reflection.
		m_shaderArguments.push_back(L"-Qstrip_reflect");
		m_shaderArguments.push_back(L"-Fre"); //-- Output reflection to the given file
		m_shaderArguments.push_back(reflectionPath.data());

		//-- Generate outout object file.
		m_shaderArguments.push_back(L"-Fo"); //-- Output object file
		m_shaderArguments.push_back(objectPath.data());

		//-- Setup an entry point and target.
		m_shaderArguments.push_back(L"-E");
		m_shaderArguments.push_back(kEntryPoints[static_cast<uint8_t>(type)]);

		m_shaderArguments.push_back(L"-T");
		m_shaderArguments.push_back(kTargets[static_cast<uint8_t>(type)]);
	}

	HRESULT ok = S_OK;
#if 1
	//-- Open source file.
	ComPtr<IDxcBlobEncoding> source = nullptr;
	ok = m_utils->CreateBlob(blob.data(), static_cast<UINT32>(blob.size()), DXC_CP_ACP, &source);
	//ok = m_utils->LoadFile(wPath.data(), nullptr, &source);
	ENGINE_ASSERT(SUCCEEDED(ok), fmt::format("[ShaderCompiler]: Can't load the file '{}'", absolutePath));

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = source->GetBufferPointer();
	sourceBuffer.Size = source->GetBufferSize();
	sourceBuffer.Encoding = DXC_CP_ACP; //-- Assume BOM says UTF8 or UTF16 or this is ANSI text.
#else
	ComPtr<IDxcBlobEncoding> source;
	m_utils->CreateBlob(pShaderSource, shaderSourceSize, CP_UTF8, source.GetAddressOf());

	DxcBuffer sourceBuffer;
	sourceBuffer.Ptr = source->GetBufferPointer();
	sourceBuffer.Size = source->GetBufferSize();
	sourceBuffer.Encoding = 0;
#endif

	ComPtr<IDxcResult> result;
	ok = m_compiler->Compile(&sourceBuffer, m_shaderArguments.data(), static_cast<UINT32>(m_shaderArguments.size()),
		m_includeHandler.Get(), IID_PPV_ARGS(&result));
	result->GetStatus(&ok);

	//-- Errors.
	{
		ComPtr<IDxcBlobUtf8> errorMsgs;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errorMsgs), nullptr);
		if (errorMsgs && errorMsgs->GetStringLength())
		{
			logger().error(fmt::format("[ShaderCompiler]: Can't compile the shader '{}'. Errors: {}", absolutePath, errorMsgs->GetStringPointer()));
		}
	}

	//-- Output object.
	{
		auto& shader = resource.m_shaders[static_cast<uint8_t>(type)];

		ComPtr<IDxcBlobUtf16> shaderName = nullptr;
		ok = result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), &shaderName);
		ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't extract object info of the shader.");

		//-- Save it.
		if (shader != nullptr)
		{
			FILE* fp = NULL;

			_wfopen_s(&fp, shaderName->GetStringPointer(), L"wb");
			fwrite(shader->GetBufferPointer(), shader->GetBufferSize(), 1, fp);
			fclose(fp);
		}
	}

	//-- Debug.
	{
		ComPtr<IDxcBlob> debugData;
		ComPtr<IDxcBlobUtf16> debugDataPath;
		//-- So if you want to save the PDBs to a separate file, use this name so that Pix will know where to find it.
		ok = result->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(debugData.GetAddressOf()), debugDataPath.GetAddressOf());
		ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't extract PDB info for the shader.");

		//-- Save it.
		{
			FILE* fp = NULL;

			//-- Note that if you don't specify -Fd, a pdb name will be automatically generated.
			//-- Use this file name to save the pdb so that PIX can find it quickly.
			_wfopen_s(&fp, debugDataPath->GetStringPointer(), L"wb");
			fwrite(debugData->GetBufferPointer(), debugData->GetBufferSize(), 1, fp);
			fclose(fp);
		}

		//-- Demonstrate getting the hash from the PDB blob using the IDxcUtils::GetPDBContents API
		ComPtr<IDxcBlob> pHashDigestBlob = nullptr;
		ComPtr<IDxcBlob> pDebugDxilContainer = nullptr;
		if (SUCCEEDED(m_utils->GetPDBContents(debugData.Get(), &pHashDigestBlob, &pDebugDxilContainer)))
		{
			// This API returns the raw hash digest, rather than a DxcShaderHash structure.
			// This will be the same as the DxcShaderHash::HashDigest returned from
			// IDxcResult::GetOutput(DXC_OUT_SHADER_HASH, ...).
			/*wprintf(L"Hash from PDB: ");
			const BYTE* pHashDigest = (const BYTE*)pHashDigestBlob->GetBufferPointer();
			assert(pHashDigestBlob->GetBufferSize() == 16); // hash digest is always 16 bytes.
			for (int i = 0; i < pHashDigestBlob->GetBufferSize(); i++)
				wprintf(L"%.2x", pHashDigest[i]);
			wprintf(L"\n");*/

			// The pDebugDxilContainer blob will contain a DxilContainer formatted
			// binary, but with different parts than the pShader blob retrieved
			// earlier.
			// The parts in this container will vary depending on debug options and
			// the compiler version.
			// This blob is not meant to be directly interpreted by an application.
		}
	}

	//-- Reflection.
	{
		ComPtr<IDxcBlob> pReflectionData;
		ok = result->GetOutput(DXC_OUT_REFLECTION, IID_PPV_ARGS(pReflectionData.GetAddressOf()), nullptr);
		ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't extract reflection info for the shader.");

		DxcBuffer reflectionBuffer;
		reflectionBuffer.Ptr = pReflectionData->GetBufferPointer();
		reflectionBuffer.Size = pReflectionData->GetBufferSize();
		reflectionBuffer.Encoding = 0;
		ComPtr<ID3D12ShaderReflection> reflection;
		ok = m_utils->CreateReflection(&reflectionBuffer, IID_PPV_ARGS(reflection.GetAddressOf()));
		ENGINE_ASSERT(SUCCEEDED(ok), "[ShaderCompiler]: Can't create reflection for the shader.");

		//-- https://github.com/kymani37299/ForwardPlusRenderer/blob/master/Engine/Render/Shader.cpp
		//-- https://rtarun9.github.io/blogs/shader_reflection/
		D3D12_SHADER_DESC desc;
		reflection->GetDesc(&desc);

		D3D12_SHADER_BUFFER_DESC cbBufferDesc;
		D3D12_SHADER_VARIABLE_DESC cbVarDesc;
		D3D12_SHADER_TYPE_DESC cbVarTypeDesc;
		for (UINT i = 0; i < desc.ConstantBuffers; ++i)
		{
			[[maybe_unused]] auto* cbReflected = reflection->GetConstantBufferByIndex(i);

			cbReflected->GetDesc(&cbBufferDesc);
			for (UINT v = 0; v < cbBufferDesc.Variables; ++v)
			{
				auto* varReflected = cbReflected->GetVariableByIndex(v);
				varReflected->GetDesc(&cbVarDesc);

				auto* varReflectType = varReflected->GetType();
				varReflectType->GetDesc(&cbVarTypeDesc);
			}
			__nop();
		}

		D3D12_SIGNATURE_PARAMETER_DESC inputSignatureDesc;
		for (UINT i = 0; i < desc.InputParameters; ++i)
		{
			reflection->GetInputParameterDesc(i, &inputSignatureDesc);
			__nop();
		}

		D3D12_SIGNATURE_PARAMETER_DESC outputSignatureDesc;
		for (UINT i = 0; i < desc.OutputParameters; ++i)
		{
			reflection->GetOutputParameterDesc(i, &outputSignatureDesc);
			__nop();
		}

		D3D12_SHADER_INPUT_BIND_DESC bindDesc;
		for (UINT i = 0; i < desc.BoundResources; ++i)
		{
			reflection->GetResourceBindingDesc(i, &bindDesc);
		}

		[[maybe_unused]] auto numInterfaceSlots = reflection->GetNumInterfaceSlots(); //-- ToDo: What the hell is it?
	}

	//-- Hash.
	{
		ComPtr<IDxcBlob> hash;
		ok = result->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(hash.GetAddressOf()), nullptr);
		//ENGINE_ASSERT(SUCCEEDED(ok), "Can't create extract PDB info for the shader.");
		if (SUCCEEDED(ok))
		{
			DxcShaderHash* pHashBuf = (DxcShaderHash*)hash->GetBufferPointer();
			std::string stringHash = "[ShaderCompiler]: Hash: ";
			for (int i = 0; i < _countof(pHashBuf->HashDigest); i++)
			{
				stringHash = fmt::format("{}{:x}", stringHash, pHashBuf->HashDigest[i]);
			}
			logger().debug(stringHash);
		}
	}

	if (FAILED(ok))
	{
		return false;
	}

	return true;
}

} //-- engine::render::d3d12.
