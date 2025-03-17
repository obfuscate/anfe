#include <engine/render/d3d12/d3d12_backend.h>
#include <engine/assert.h>

using Microsoft::WRL::ComPtr;

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
		while (!factory->EnumAdapterByGpuPreference(adapterIndex, preference, IID_PPV_ARGS(&adapter)))
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

} //-- unnamed.


bool Backend::initialize(const Desc& desc)
{
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

	//-- Describe and create the swap chain.
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = desc.numBuffers;
	swapChainDesc.Width = desc.width;
	swapChainDesc.Height = desc.height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //-- ToDo: Use sRGB?
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
		//-- Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = desc.numBuffers;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ok = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a descriptor heap for the backbuffers.");

		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	//-- Create frame resources (a RTV for each frame).
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

		m_renderTargets.resize(desc.numBuffers);
		for (UINT n = 0; n < desc.numBuffers; n++)
		{
			ok = m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
			ENGINE_ASSERT(SUCCEEDED(ok));

			m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, m_rtvDescriptorSize);
		}
	}

	ok = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a command allocator.");

	//-- Create the command list.
	ok = m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a command list.");

	//-- Command lists are created in the recording state, but there is nothing to record yet.
	//-- The main loop expects it to be closed, so close it now.
	ok = m_commandList->Close();
	ENGINE_ASSERT(SUCCEEDED(ok), "Can't close a command list.");

	//-- Create synchronization objects.
	{
		ok = m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
		ENGINE_ASSERT(SUCCEEDED(ok), "Can't create a fence.");
		m_fenceValue = 1;

		//-- Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ok = HRESULT_FROM_WIN32(GetLastError());
			ENGINE_ASSERT(SUCCEEDED(ok));
		}
	}

	return true;
}


void Backend::release()
{
	waitForPreviousFrame();

	CloseHandle(m_fenceEvent);
	m_device.Reset();
}


void Backend::waitForPreviousFrame()
{
	//-- WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	//-- This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	//-- sample illustrates how to use fences for efficient resource usage and to
	//-- maximize GPU utilization.
	//-- ToDo: Reconsider later.

	//-- Signal and increment the fence value.
	const UINT64 fence = m_fenceValue;
	HRESULT ok = m_graphicsCommandQueue->Signal(m_fence.Get(), fence);
	ENGINE_ASSERT_DEBUG(SUCCEEDED(ok), "Can't get a signal from a command queue");
	m_fenceValue++;

	// Wait until the previous frame is finished.
	if (m_fence->GetCompletedValue() < fence)
	{
		ok = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}


void Backend::present()
{
	//-- RENDER PART. TODO: MOVE OUT TO THE SYSTEMS.
	{
		//-- We use a single command allocator to manage the memory space where drawing commands for both buffers in the swap chain are recorded.
		//-- This implies that we need to flush the command queue before recording the commands to create and present a new frame,
		//-- as all commands are recorded in the same memory space regardless of the frame we are creating
		//-- - we can't overwrite commands still in use by the GPU, obviously.
		
		//-- Command list allocators can only be reset when the associated 
		//-- command lists have finished execution on the GPU; apps should use 
		//-- fences to determine GPU execution progress.
		HRESULT ok = m_commandAllocator->Reset();
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));

		//-- However, when ExecuteCommandList() is called on a particular command list,
		//-- that command list can then be reset at any time and must be before re-recording.
		ok = m_commandList->Reset(m_commandAllocator.Get(), nullptr);
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));

		//-- Indicate that the back buffer will be used as a render target.
		auto beginBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &beginBarriers);

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), m_frameIndex, m_rtvDescriptorSize);

		//-- Record commands.
		const float clearColor[] = { 0.2f, 0.8f, 0.4f, 1.0f };
		m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

		//-- Indicate that the back buffer will now be used to present.
		auto endBarriers = CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &endBarriers);

		ok = m_commandList->Close();
		ENGINE_ASSERT_DEBUG(SUCCEEDED(ok));
	}

	//-- Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	m_graphicsCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	//-- Present the frame.
	HRESULT ok = m_swapChain->Present(1, 0); //-- ToDo: Rework on v-sync param.
	ENGINE_ASSERT_DEBUG(SUCCEEDED(ok), "Can't present the frame.");

	waitForPreviousFrame();
}

} //-- engine::render::d3d12.
