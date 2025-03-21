#include <engine/render/d3d12/backend.h>
#include <engine/assert.h>
#include <engine/helpers.h>
#include <engine/math.h>

using Microsoft::WRL::ComPtr;
using namespace std::string_view_literals;

namespace engine::render::d3d12
{


namespace
{

inline D3D_FEATURE_LEVEL createFakeDevice(IDXGIAdapter1* adapter, const std::vector<D3D_FEATURE_LEVEL>& desiredFeatureLevels)
{
	auto result = D3D_FEATURE_LEVEL_1_0_CORE;

	DXGI_ADAPTER_DESC1 desc;
	adapter->GetDesc1(&desc);

	if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
	{
		//-- Don't select the Basic Render Driver adapter.
		return result;
	}

	//-- Check to see whether the adapter supports Direct3D 12, but don't create the actual device yet.
	for (auto fl : desiredFeatureLevels)
	{
		if (SUCCEEDED(D3D12CreateDevice(adapter, fl, _uuidof(ID3D12Device), nullptr)))
		{
			result = fl;
			break;
		}
	}

	return result;
}


//-- Also returns max possible FL.
D3D_FEATURE_LEVEL getHardwareAdapter(
	IDXGIFactory7* pFactory,
	IDXGIAdapter1** ppAdapter,
	bool requestHighPerformanceAdapter,
	const std::vector<D3D_FEATURE_LEVEL>& desiredFeatureLevels)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory7> factory;

	D3D_FEATURE_LEVEL maxFeatureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory))))
	{
		const auto preference = requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED;
		UINT adapterIndex = 0;
		while (SUCCEEDED(factory->EnumAdapterByGpuPreference(adapterIndex, preference, IID_PPV_ARGS(&adapter))))
		{
			maxFeatureLevel = createFakeDevice(adapter.Get(), desiredFeatureLevels);

			if (maxFeatureLevel != D3D_FEATURE_LEVEL_1_0_CORE)
			{
				break;
			}

			++adapterIndex;
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
		{
			maxFeatureLevel = createFakeDevice(adapter.Get(), desiredFeatureLevels);
		}
	}

	*ppAdapter = adapter.Detach();

	return maxFeatureLevel;
}

constexpr uint32_t kWidth = 256;
constexpr uint32_t kHeight = 256;
constexpr uint32_t kPixelSize = 4;

//-- Generate a simple black and white checkerboard texture.
[[maybe_unused]] std::vector<UINT8> generateTextureData()
{
	//-- rowPitch is the byte size of a row in the texture.
	constexpr UINT rowPitch = kWidth * kPixelSize;
	constexpr UINT cellPitch = rowPitch >> 3; // The width of a cell in the checkerboard texture.
	constexpr UINT cellHeight = kWidth >> 3; // The height of a cell in the checkerboard texture.
	constexpr UINT textureSize = rowPitch * kHeight;

	std::vector<UINT8> data(textureSize);
	UINT8* pData = &data[0];

	for (UINT n = 0; n < textureSize; n += kPixelSize)
	{
		UINT x = n % rowPitch;
		UINT y = n / rowPitch;
		UINT i = x / cellPitch;
		UINT j = y / cellHeight;

		if (i % 2 == j % 2)
		{
			pData[n] = 0x00;        // R
			pData[n + 1] = 0x00;    // G
			pData[n + 2] = 0x00;    // B
			pData[n + 3] = 0xff;    // A
		}
		else
		{
			pData[n] = 0xff;        // R
			pData[n + 1] = 0xff;    // G
			pData[n + 2] = 0xff;    // B
			pData[n + 3] = 0xff;    // A
		}
	}

	return data;
}


inline size_t calculateConstantBufferByteSize(size_t byteSize)
{
	// Constant buffer size is required to be aligned.
	return (byteSize + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1);
}

} //-- unnamed.


bool Backend::initialize(const Desc& desc)
{
	m_shaderCompiler.initialize();

	uint32_t dxgiFactoryFlags = 0;

	//-- Enable the debug layer (requires the Graphics Tools "optional feature").
	//-- NOTE: Enabling the debug layer after device creation will invalidate the active device.
	if (hasFlag(desc.flags, Flags::DebugLayer))
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			//-- Enable additional debug layers.
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}

	ComPtr<IDXGIFactory7> factory;
	HRESULT ok = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a DXGI Factory.");

	ComPtr<IDXGIAdapter1> hardwareAdapter;
	std::vector<D3D_FEATURE_LEVEL> requestedFeatureLevels = { D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0 };
	auto featureLevel = getHardwareAdapter(factory.Get(), &hardwareAdapter, true, requestedFeatureLevels);

	ok = D3D12CreateDevice(hardwareAdapter.Get(), featureLevel, IID_PPV_ARGS(&m_device));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a D3D12 device.");

	//-- Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ok = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_graphicsCommandQueue));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a direct command queue.");

	//-- Viewport.
	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(desc.width), static_cast<float>(desc.height), 0.0f, 1.0f);
	m_scissorRect = CD3DX12_RECT(0, 0, desc.width, desc.height);

	//-- Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = desc.numBuffers;
	swapChainDesc.Width = desc.width;
	swapChainDesc.Height = desc.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //-- ToDo: Use sRGB?
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //-- we want to use the flip model to present frames on the screen.
	swapChainDesc.SampleDesc.Count = 1;

	HWND wnd = static_cast<HWND>(desc.hwnd);
	ComPtr<IDXGISwapChain1> swapChain;
	ok = factory->CreateSwapChainForHwnd(m_graphicsCommandQueue.Get(), wnd, &swapChainDesc,
		nullptr, nullptr, &swapChain);
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a swap chain for HWND.");

	//-- does not support fullscreen transitions. ToDo: Reconsider later.
	ok = factory->MakeWindowAssociation(wnd, DXGI_MWA_NO_ALT_ENTER);
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't make window association.");

	ok = swapChain.As(&m_swapChain);
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't assign a swap chain.");

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//-- Create descriptor heaps.
	{
		//-- RTV
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = desc.numBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ok = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a descriptor heap for the backbuffers.");

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		//-- CBV SRV UAV
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
		cbvHeapDesc.NumDescriptors = 3; //-- ToDo: Reconsider later.
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		ok = m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a descriptor heap for CBV, SRV, UAV.");

		m_cbvSrvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//-- DSV
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ok = m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a descriptor heap for DSV.");
	}

	//-- Create frame resources (a RTV for each frame).
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		m_renderTargets.resize(desc.numBuffers);
		m_frameCommandAllocators.resize(desc.numBuffers);
		m_fenceValues.resize(desc.numBuffers);
		for (UINT i = 0; i < desc.numBuffers; i++)
		{
			ok = m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
			ENGINE_ASSERT(SUCCEEDED(ok));

			m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			ok = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frameCommandAllocators[i]));
			ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a command allocator.");
		}
	}

	//-- Create the depth stencil view.
	//-- Performance tip: Deny shader resource access to resources that don't need shader resource views.
	{
		constexpr auto kDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		auto depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(kDepthFormat, desc.width, desc.height,
			1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
		auto clearValue = CD3DX12_CLEAR_VALUE(kDepthFormat, 1.0f, 0);

		ok = m_device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &depthDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, IID_PPV_ARGS(&m_depthStencil));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a depth-stencil buffer.");

		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {
			.Format = kDepthFormat,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE
		};
		m_device->CreateDepthStencilView(m_depthStencil.Get(), &depthStencilDesc, m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	ok = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_BUNDLE, IID_PPV_ARGS(&m_bundleAllocator));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a bundle command allocator.");

	//-- Create the command list.
	ok = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_frameCommandAllocators[m_frameIndex].Get(), nullptr, IID_PPV_ARGS(&m_commandList));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a command list.");

	//-- Command lists are created in the recording state, but there is nothing to record yet.
	//-- The main loop expects it to be closed, so close it now.
	//ok = m_commandList->Close();
	//ENGINE_ASSERT(SUCCEEDED(ok), "Can't close a command list.");

	//-- Create a root signature.
	{
		CD3DX12_DESCRIPTOR_RANGE1 ranges[2];
		CD3DX12_ROOT_PARAMETER1 rootParameters[4];

		//-- Global.
		rootParameters[0].InitAsConstantBufferView(0, 0);

		//-- Per camera.
		//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[1].InitAsConstantBufferView(1, 0);
		//rootParameters[1].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		//-- Per Object.
		rootParameters[2].InitAsConstantBufferView(2, 0);
		//ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		//rootParameters[2].InitAsDescriptorTable(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX);

		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
		rootParameters[3].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);

		//-- static samplers are part of a root signature, but do not count towards the 64 DWORD limit.
		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		sampler.MipLODBias = 0;
		sampler.MaxAnisotropy = 0;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.MinLOD = 0.0f;
		sampler.MaxLOD = D3D12_FLOAT32_MAX;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		//-- Allow input layout and deny uneccessary access to certain pipeline stages.
		D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;

		//-- The serialized version could be stored in a file on disk for quick loading, eliminating the need to recreate it each time.
		//-- ToDo: Root signatures can also be defined directly within shader code.
		//-- In such cases, the shader code and the root signature are compiled together into the same memory blob.
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

			//-- This is the highest version the sample supports. If CheckFeatureSupport succeeds, the HighestVersion returned will not be greater than this.
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

			if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
			{
				featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			ok = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &signature, &error);
			ENGINE_ASSERT(SUCCEEDED(ok));
		}

		ok = m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
		ENGINE_ASSERT(SUCCEEDED(ok));
	}

	//-- Create the pipeline state, which includes compiling and loading shaders.
	{
		auto shader = m_shaderCompiler.compile("/shaders/test_shader.hlsl"sv);
		if (!shader->ready())
		{
			ENGINE_FAIL("Can't load the test shader");
		}

		//-- Define the vertex input layout.
		//-- ToDo: Use reflection.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, //-- ToDo: pack TBN.
			{ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, //-- ToDo: Use r16g16.
			{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 5, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }, //-- ToDo: Use r16g16.
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 6, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		auto vertexShader = shader->shader(resources::ShaderResource::Type::Vertex);
		auto pixelShader = shader->shader(resources::ShaderResource::Type::Pixel);

		//-- Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.first, vertexShader.second);
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.first, pixelShader.second);
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ok = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a PSO.");

		shader->release(); //-- release IDxcBlob memory. Todo: Reconsider later.
	}

	//-- Create the constant buffers.
	{
		//-- Per Camera.
		const D3D12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		size_t cbSize = 2 * desc.numBuffers * calculateConstantBufferByteSize(sizeof(PerCameraCB));

		D3D12_RESOURCE_DESC constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
		assertIfFailed(m_device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_perCameraConstants.ReleaseAndGetAddressOf())));

		assertIfFailed(m_perCameraConstants->Map(0, nullptr, &m_perCameraConstantsMapped));

		// GPU virtual address of the resource
		m_perCameraConstantsAddress = m_perCameraConstants->GetGPUVirtualAddress();

		//-- Per Object.
		cbSize = 2 * desc.numBuffers * calculateConstantBufferByteSize(sizeof(PerObjectCB));

		constantBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(cbSize);
		assertIfFailed(m_device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(m_perObjectConstants.ReleaseAndGetAddressOf())));

		assertIfFailed(m_perObjectConstants->Map(0, nullptr, &m_perObjectConstantsMapped));

		// GPU virtual address of the resource
		m_perObjectConstantsAddress = m_perObjectConstants->GetGPUVirtualAddress();
	}

	//-- Create and record the bundle.
	{
		ok = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_BUNDLE, m_bundleAllocator.Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_bundleCommands));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a bundle command list");

		/*m_bundleCommands->SetGraphicsRootSignature(m_rootSignature.Get());
		m_bundleCommands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_bundleCommands->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_bundleCommands->IASetIndexBuffer(&m_indexBufferView);
		m_bundleCommands->DrawIndexedInstanced(36, 1, 0, 0, 0);
		assertIfFailed(m_bundleCommands->Close());*/
	}

	//-- Note: ComPtr's are CPU objects but this resource needs to stay in scope until
	//-- the command list that references it has finished executing on the GPU.
	//-- We will flush the GPU at the end of this method to ensure the resource is not
	//-- prematurely destroyed.
	ComPtr<ID3D12Resource> textureUploadHeap;

	//-- Create the texture.
	{
		//-- Describe and create a Texture2D.
		D3D12_RESOURCE_DESC textureDesc = {};
		textureDesc.MipLevels = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.Width = kWidth;
		textureDesc.Height = kHeight;
		textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		textureDesc.DepthOrArraySize = 1;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
		assertIfFailed(m_device->CreateCommittedResource(&defaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_testTexture)));

		//-- Well, the intermediate resource on the upload heap is only used to store the texture data.
		//-- That is, we don't need to create another texture, a buffer is more than enough.
		//-- And indeed, CD3DX12_RESOURCE_DESC::Buffer is used to create a D3D12_RESOURCE_DESC that describe a buffer.
		//-- GetRequiredIntermediateSize takes a ID3DResource to compute the amount of memory used by the related resource.
		//-- That way, we can pass the texture on the default heap to calculate the necessary memory size to allocate on the upload heap.
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_testTexture.Get(), 0, 1);

		//-- Create the GPU upload buffer.
		auto uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto uploadBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
		assertIfFailed(m_device->CreateCommittedResource(&uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&textureUploadHeap)));

		//-- Copy data to the intermediate upload heap and then schedule a copy  from the upload heap to the Texture2D.
		std::vector<UINT8> texture = generateTextureData();

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &texture[0];
		textureData.RowPitch = kWidth * kPixelSize; //-- RowPitch is the byte size of a row of the subresource.
		textureData.SlicePitch = textureData.RowPitch * kHeight; //-- SlicePitch is the byte size of the whole subresource (at least for 2D subresources; for 3D subresources is the byte size of the depth)

		UpdateSubresources(m_commandList.Get(), m_testTexture.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_testTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		m_commandList->ResourceBarrier(1, &barrier);

		//-- Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		//-- Shader4ComponentMapping indicates what values from memory should be returned when the texture is accessed in a shader via this shader resource view (SRV).
		//-- It allows to select how memory gets routed to the four return components in a shader after a memory fetch.
		//-- The options for each shader component [0..3] (corresponding to RGBA) are: component [0..3] from the SRV fetch result or force 0 or 1.
		//-- In other words, you can map a channel value to another channel, or set it to 0 or 1, when you read a texel of the texture by using this view.
		//-- The default 1:1 mapping can be specified with D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING, indicating you want (Red, Green, Blue, Alpha) as usual;
		//-- that is, (component 0, component 1, component 2, component 3).
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart(), 1, m_cbvSrvUavDescriptorSize);
		m_device->CreateShaderResourceView(m_testTexture.Get(), &srvDesc, srvHandle);
		//-- ToDo: Make a wrapper for SRV.
	}

	//-- TODO REMOVE
	{
		//-- Initialize the world matrix
		m_worldMatrix = math::matrix::Identity;

		//-- Initialize the view matrix
		const math::vec3 eye = { 0.0f, 3.0f, -10.0f };
		const math::vec3 at = { 0.0f, 1.0f, 0.0f };
		const math::vec3 up = { 0.0f, 1.0f, 0.0f };
		m_viewMatrix = math::matrix::CreateLookAt(eye, at, up); //-- LH?

		//-- Initialize the projection matrix
		m_projectionMatrix = math::matrix::CreatePerspectiveFieldOfView(DirectX::XM_PIDIV4, desc.width / static_cast<float>(desc.height), 0.01f, 100.0f); //-- LH?
	}

	m_meshResource = std::make_shared<resources::MeshResource>();
	m_meshResource->load("/meshes/max7_blend_cube_24.obj");
	{
		std::array<D3D12_RESOURCE_BARRIER, static_cast<size_t>(resources::MeshResource::Stream::Count) + 1> barriers;
		//-- Prepare buffers for copying.
		for (size_t i = 0; i < barriers.size() - 1; ++i)
		{
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_meshResource->m_streams[i].Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		}
		barriers[7] = CD3DX12_RESOURCE_BARRIER::Transition(m_meshResource->m_indexBuffer.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
		m_commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());

		for (size_t i = 0; i < barriers.size() - 1; ++i)
		{
			m_commandList->CopyBufferRegion(m_meshResource->m_streams[i].Get(), 0, m_meshResource->m_uploadBuffers[i].Get(), 0, m_meshResource->m_streamsSize[i]);
		}
		m_commandList->CopyBufferRegion(m_meshResource->m_indexBuffer.Get(), 0, m_meshResource->m_uploadIndexBuffer.Get(), 0, m_meshResource->m_indexBufferSize);

		//-- Set buffers to the right state.
		for (size_t i = 0; i < barriers.size() - 1; ++i)
		{
			barriers[i] = CD3DX12_RESOURCE_BARRIER::Transition(m_meshResource->m_streams[i].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		}
		barriers[7] = CD3DX12_RESOURCE_BARRIER::Transition(m_meshResource->m_indexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		m_commandList->ResourceBarrier(static_cast<UINT>(barriers.size()), barriers.data());
	}

	//-- Close the command list and execute it to begin the initial GPU setup.
	assertIfFailed(m_commandList->Close());
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//-- Create synchronization objects.
	{
		ok = m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a fence.");
		m_fenceValues[m_frameIndex]++;

		//-- Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ok = HRESULT_FROM_WIN32(GetLastError());
			ENGINE_ASSERT(SUCCEEDED(ok));
		}

		//-- Wait for the command list to execute; we are reusing the same command 
		//-- list in our main loop but for now, we just want to wait for setup to 
		//-- complete before continuing.
		waitForGPU();
	}

	return true;
}


void Backend::release()
{
	waitForGPU();

	CloseHandle(m_fenceEvent);
	m_device.Reset();
}

//-- Some overview of frame buffering:
//-- * https://paminerva.github.io/docs/LearnDirectX/01.F-Hello-Frame-Buffering
void Backend::waitForGPU()
{
	//-- Schedule a Signal command in the queue.
	assertIfFailed(m_graphicsCommandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]));

	//-- Wait until the fence has been processed.
	assertIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
	WaitForSingleObjectEx(m_fenceEvent, 1000, FALSE);

	//-- Increment the fence value for the current frame.
	m_fenceValues[m_frameIndex]++;
}


void Backend::moveToNextFrame()
{
	//-- Schedule a Signal command in the queue.
	auto currentFenceValue = m_fenceValues[m_frameIndex];
	assertIfFailed(m_graphicsCommandQueue->Signal(m_fence.Get(), currentFenceValue));

	//-- Update the frame index.
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	//-- If the next frame is not ready to be rendered yet, wait until it is ready.
	if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
	{
		assertIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent));
		WaitForSingleObjectEx(m_fenceEvent, 1000, FALSE);
	}

	//-- Set the fence value for the next frame.
	m_fenceValues[m_frameIndex] = currentFenceValue + 1;
}


void Backend::present()
{
	//-- UPDATE. REMOVE.
	{
		const float rotationSpeed = 0.015f;

		// Update the rotation constant
		m_curRotationAngleRad += rotationSpeed;
		if (m_curRotationAngleRad >= math::k2Pi)
		{
			m_curRotationAngleRad -= math::k2Pi;
		}

		// Rotate the cube around the Y-axis
		auto rot = math::matrix::CreateRotationY(m_curRotationAngleRad);
		auto scale = math::matrix::CreateScale(0.05f);
		m_worldMatrix = scale * rot; //-- ToDo: Just create a ctor.
		//math::matrix::Create
	}

	//-- RENDER PART. TODO: MOVE OUT TO THE SYSTEMS.
	{
		//-- We use a single command allocator to manage the memory space where drawing commands for both buffers in the swap chain are recorded.
		//-- This implies that we need to flush the command queue before recording the commands to create and present a new frame,
		//-- as all commands are recorded in the same memory space regardless of the frame we are creating
		//-- - we can't overwrite commands still in use by the GPU, obviously.
		
		//-- Command list allocators can only be reset when the associated 
		//-- command lists have finished execution on the GPU; apps should use 
		//-- fences to determine GPU execution progress.
		HRESULT ok = m_frameCommandAllocators[m_frameIndex]->Reset();
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));

		//-- However, when ExecuteCommandList() is called on a particular command list,
		//-- that command list can then be reset at any time and must be before re-recording.
		ok = m_commandList->Reset(m_frameCommandAllocators[m_frameIndex].Get(), nullptr);
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
		m_commandList->SetPipelineState(m_pipelineState.Get()); //-- Or we can reset to this PSO.

		//-- Root Signature.
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

		ID3D12DescriptorHeap* ppHeaps[] = { m_cbvSrvUavHeap.Get() };
		m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		unsigned int constantBufferIndex = 2 * (m_frameIndex % 2);
		D3D12_GPU_VIRTUAL_ADDRESS basePerObjectCBAddress = 0;
		{
			// Index into the available constant buffers based on the number
			// of draw calls. We've allocated enough for a known number of
			// draw calls per frame times the number of back buffers

			{
				// Set the per-camera constants
				PerCameraCB cbParameters = {};

				// Shaders compiled with default row-major matrices
				//XMStoreFloat4x4(&cbParameters.worldMatrix, XMMatrixTranspose(m_worldMatrix));
				XMStoreFloat4x4(&cbParameters.view, m_viewMatrix);
				XMStoreFloat4x4(&cbParameters.proj, m_projectionMatrix);

				auto offset = calculateConstantBufferByteSize(sizeof(cbParameters)) * constantBufferIndex;
				// Set the constants for the first draw call
				memcpy((uint8_t*)m_perCameraConstantsMapped + offset, &cbParameters, sizeof(cbParameters));

				// Bind the constants to the shader
				auto baseGpuAddress = m_perCameraConstantsAddress + offset;
				m_commandList->SetGraphicsRootConstantBufferView(1, baseGpuAddress);
			}

			{
				// Set the per-camera constants
				PerObjectCB cbParameters = {};

				// Shaders compiled with default row-major matrices
				XMStoreFloat4x4(&cbParameters.world, m_worldMatrix);


				// Set the constants for the first draw call
				auto offset = calculateConstantBufferByteSize(sizeof(cbParameters)) * constantBufferIndex;
				memcpy((uint8_t*)m_perObjectConstantsMapped + offset, &cbParameters, sizeof(cbParameters));

				// Bind the constants to the shader
				basePerObjectCBAddress = m_perObjectConstantsAddress + offset;
				m_commandList->SetGraphicsRootConstantBufferView(2, basePerObjectCBAddress);
			}
		}

		//m_commandList->SetGraphicsRootDescriptorTable(0, 0);
		//m_commandList->SetGraphicsRootDescriptorTable(1, m_perCameraConstantsAddress);
		//m_commandList->SetGraphicsRootDescriptorTable(2, m_perObjectConstantsAddress);
		//m_commandList->SetGraphicsRootDescriptorTable(3, m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart());

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(m_cbvSrvUavHeap->GetGPUDescriptorHandleForHeapStart(), 1, m_cbvSrvUavDescriptorSize);
		m_commandList->SetGraphicsRootDescriptorTable(3, srvHandle);

		//-- Indicate that the back buffer will be used as a render target.
		auto beginBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &beginBarriers);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize); //-- Looks like baked RTV. ToDo: Make a wrapper.
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		//-- Record commands.
		const float clearColor[] = { 0.2f, 0.8f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
		m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		//m_commandList->ExecuteBundle(m_bundleCommands.Get());
		if (m_meshResource->ready())
		{
			m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			for (auto& submesh : m_meshResource->m_subMeshes)
			{
				//-- ToDo: We may store streamviews and indexbuffer outside of submesh and setup it once, instead of per submesh.
				m_commandList->IASetVertexBuffers(0, static_cast<UINT>(submesh.renderPart.streamViews.size()), submesh.renderPart.streamViews.data());
				m_commandList->IASetIndexBuffer(&submesh.renderPart.indexBufferView);
				m_commandList->DrawIndexedInstanced(submesh.renderPart.numIndices, 1, submesh.renderPart.startIndex, submesh.renderPart.baseVertex, 0);
			}
		}

		/*{
			PerObjectCB cbParameters;

			basePerObjectCBAddress += calculateConstantBufferByteSize(sizeof(PerObjectCB));
			++constantBufferIndex;

			// Update the World matrix of the second cube
			math::matrix scaleMatrix = math::matrix::CreateScale(0.2f);
			math::matrix rotationMatrix = math::matrix::CreateRotationY(-2.0f * m_curRotationAngleRad);
			math::matrix translateMatrix = math::matrix::CreateTranslation(0.0f, 0.0f, -5.0f);

			// Update the world variable to reflect the current light
			XMStoreFloat4x4(&cbParameters.world, (scaleMatrix * translateMatrix) * rotationMatrix);

			// Set the constants for the draw call
			auto offset = calculateConstantBufferByteSize(sizeof(cbParameters)) * constantBufferIndex;
			memcpy((uint8_t*)m_perObjectConstantsMapped + offset, &cbParameters, sizeof(PerObjectCB));

			// Bind the constants to the shader
			m_commandList->SetGraphicsRootConstantBufferView(2, basePerObjectCBAddress);

			// Draw the second cube
			//m_commandList->ExecuteBundle(m_bundleCommands.Get()); //-- Or just Draw(...).
			if (m_meshResource->ready())
			{
				auto& renderPart = m_meshResource->renderRepresentation();
				m_commandList->DrawIndexedInstanced(renderPart.numIndices, 1, renderPart.startIndex, renderPart.baseVertex, 0);
			}
		}*/

		//-- Indicate that the back buffer will now be used to present.
		auto endBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &endBarriers);

		ok = m_commandList->Close(); //-- Command list must be close before submitting it to a command queue.
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
	}

	//-- Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//-- Present the frame.
	HRESULT ok = m_swapChain->Present(1, 0); //-- ToDo: Rework on v-sync param.
	ENGINE_ASSERT_DEBUG(SUCCEEDED(ok), "Can't present the frame.");

	moveToNextFrame();
}

//-- Binding slots are not physical blocks of memory or registers that a GPU can access to read descriptors.
//-- They are simple names (character strings) used to associate descriptors to resource declarations in shader programs

//-- GPUs have access to four types of memory :
//--
//-- 1) Dedicated video memory : this is memory reserved\local to the GPU(VRAM).
//--	It's where we allocate most of the resources accessed by the GPU(through the shader programs).
//--
//-- 2) Dedicated system memory : it is a part of the dedicated video memory
//--	It's allocated at boot time and used by the GPU for internal purposes.
//--	That is, we can't use it to allocate memory from our application.
//--
//-- 3) Shared system memory : this is CPU - visible GPU memory.
//--	Usually, it is a small part of the GPU local memory(VRAM) accessible by the CPU through the PCI - e bus,
//--	but the GPU can also use CPU system memory(RAM) as GPU memory if needed.
//--	Shared system memory is often used as a source in copy operations from shared to dedicated memory
//--	(that is, from CPU - accessible memory to GPU local memory) to prevent the GPU from accessing resources in memory via the PCI - e bus.
//--	It's write - combine memory from the CPU point of view, which means that write operations are buffered up and 
//--	executed in groups when the buffer is full, or when important events occur.This allows to speed up write operations,
//--	but read ones should be avoided as write - combine memory is uncached.
//--	This means that if you try to read this memory from your CPU application,
//--	the buffer that holds the write operations need to be flushed first, which makes reads from write - combine memory slow.
//--
//-- 4) CPU system memory : it's system memory(RAM) that, like shared system memory, can be accessed from both CPU and GPU.
//--	However, CPUs can read from this memory without problems as it is cached.
//--	On the other hand, GPUs need to access this memory through the PCI - e bus, which can be a bottleneck compared to
//--	the direct memory access of CPUs through the system memory bus.
//--
//-- If you have an integrated graphics card or use a software adapter, there is no distinction between the four memory types mentioned above.
//-- In that case, both the CPU and GPU will share the only memory type available: system memory (RAM).
//-- This implies that your GPU may have limited and slower memory access.

//-- When the CreateCommittedResource function is invoked, we need to specify the type of memory where space should be allocated for
//-- the resource we want to create.You can indicate this information in two ways : abstract and custom.
//-- In the abstract way, we have three types of memory heaps that allow abstraction from the current hardware.
//--
//-- 1) Default heap : memory that resides in dedicated video memory.
//-- 2) Upload heap : memory that resides in shared video memory.
//-- 3) Readback heap : memory that resides in CPU system memory.
//--
//-- Therefore, using the abstract approach, regardless of whether you have a discrete GPU(that is, a dedicated graphics card) or
//-- an integrated one, physical memory allocations are hidden from the programmer.

} //-- engine::render::d3d12.
