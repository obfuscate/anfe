#pragma once

#include <engine/render/render_backend.h>
#include <engine/integration/d3d12/integration.h>

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
	using Resources = std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>>;

	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_graphicsCommandQueue;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtvHeap; //-- ToDo: Write a wrapper for this.
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_commandList; //-- ToDo: Move to the pool.

	Resources m_renderTargets; //-- ToDo: Make a BackBufferResource.

	UINT m_rtvDescriptorSize = 0;

	//-- Synchronization block.
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	UINT m_frameIndex;
	HANDLE m_fenceEvent;
	UINT64 m_fenceValue;
};

} //-- engine::render::d3d12.
