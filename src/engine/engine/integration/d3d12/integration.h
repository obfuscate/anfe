#pragma once

#include <directx-dxc/d3d12shader.h>
#include <directx-dxc/dxcapi.h>

#include <dxgi1_6.h>
#include <d3d12sdklayers.h>
#include <DirectXMath.h>
#include <wrl.h>

#include <engine/integration/d3d12/d3dx12.h>
#include <engine/integration/d3d12/D3D12MemAlloc.h>
#include <engine/assert.h>

inline void assertIfFailed(HRESULT hr)
{
	ENGINE_ASSERT(SUCCEEDED(hr));
}

inline void assertIfFailed(HRESULT hr, std::string_view message)
{
	ENGINE_ASSERT(SUCCEEDED(hr), message);
}
