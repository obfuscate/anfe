#pragma once

#include <engine/render/render_backend.h>
#include <engine/render/d3d12/shader_compiler.h>
#include <engine/integration/d3d12/integration.h>
#include <engine/math.h>
#include <engine/resources/mesh_resource.h>

namespace engine::render::d3d12
{

class Backend : public IBackend
{
public:
	~Backend() {}

	bool initialize(const Desc& desc) override;
	void release() override;

	void present() override;

	ID3D12Device* device() { return m_device.Get(); }

private:
	//-- Flushes the command queue.
	void waitForGPU();
	//-- Only waits until the GPU finishes drawing at least (the oldest) one of the frames queued.
	//-- In other words, it checks if we can continue creating frames on the CPU timeline.
	void moveToNextFrame();

private:
	struct PerCameraCB
	{
		math::matrix view;
		math::matrix proj;
		math::matrix viewProj;
	};

	struct PerObjectCB
	{
		math::matrix world;
	};

	using Resources = std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>;
	using CommandAllocators = std::vector<Microsoft::WRL::ComPtr<ID3D12CommandAllocator>>;

	Microsoft::WRL::ComPtr<ID3D12Device14> m_device;
	Microsoft::WRL::ComPtr<D3D12MA::Allocator> m_memoryAllocator = nullptr;

	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap; //-- ToDo: Write a wrapper for this.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbvSrvUavHeap;
	CommandAllocators m_frameCommandAllocators;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_bundleAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList; //-- ToDo: Move to the pool.
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_bundleCommands;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipelineState;

	CD3DX12_VIEWPORT m_viewport;
	CD3DX12_RECT m_scissorRect;

	Resources m_renderTargets; //-- ToDo: Make a BackBufferResource.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depthStencil;

	UINT m_rtvDescriptorSize = 0;
	UINT m_cbvSrvUavDescriptorSize = 0;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_testTexture;

	resources::MeshResourcePtr m_meshResource;

	//-- Should be part of ContanstBufferResource.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_perCameraConstants;
	D3D12_GPU_VIRTUAL_ADDRESS m_perCameraConstantsAddress;
	void* m_perCameraConstantsMapped = nullptr;

	//-- Should be part of ContanstBufferResource.
	Microsoft::WRL::ComPtr<ID3D12Resource> m_perObjectConstants;
	D3D12_GPU_VIRTUAL_ADDRESS m_perObjectConstantsAddress;
	void* m_perObjectConstantsMapped = nullptr;

	//-- Synchronization block.
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_frameIndex = 0;
	HANDLE m_fenceEvent = NULL;
	std::vector<UINT64> m_fenceValues;

	//-- ToDo: Reconsider later. Perhaps it should be part of ShaderResourceManager.
	ShaderCompiler m_shaderCompiler;

	//-- TODO REMOVE
	// Scene constants, updated per-frame
	float m_curRotationAngleRad = 0.0f;
	// These computed values will be loaded into a ConstantBuffer
	// during Render
	math::matrix m_worldMatrix;
	math::matrix m_viewMatrix;
	math::matrix m_projectionMatrix;
};

} //-- engine::render::d3d12.
