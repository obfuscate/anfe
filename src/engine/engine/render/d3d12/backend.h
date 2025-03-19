#pragma once

#include <engine/render/render_backend.h>
#include <engine/render/d3d12/shader_compiler.h>
#include <engine/integration/d3d12/integration.h>
#include <engine/math.h>

namespace engine::render::d3d12
{

class Backend : public IBackend
{
public:
	~Backend() {}

	bool initialize(const Desc& desc) override;
	void release() override;

	void present() override;

private:
	void waitForPreviousFrame();

private:
	struct GlobalConstBuffer
	{
		math::vec4 offset;
	};

	using Resources = std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>;

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap; //-- ToDo: Write a wrapper for this.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_bundleAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList; //-- ToDo: Move to the pool.
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_bundleCommands;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	Resources m_renderTargets; //-- ToDo: Make a BackBufferResource.

	UINT m_rtvDescriptorSize = 0;
	UINT m_cbvSrvUavDescriptorSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_testTexture;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_constantBuffer;
	GlobalConstBuffer m_constantBufferData;
	UINT8* m_pCbvDataBegin = nullptr;

	//-- Synchronization block.
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValue;

	//-- ToDo: Reconsider later. Perhaps it should be part of ShaderResourceManager.
	ShaderCompiler m_shaderCompiler;
};

} //-- engine::render::d3d12.
