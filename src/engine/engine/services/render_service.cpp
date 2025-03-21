#include <engine/services/render_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/input_service.h>
#include <engine/services/windows_service.h>

//-- render backends.
#include <engine/render/d3d12/backend.h>

namespace engine
{

namespace
{

META_REGISTRATION
{
	rttr::registration::enumeration<GraphicsAPI>("GraphicsAPI")
	(
		rttr::value("dx12", GraphicsAPI::DirectX12)
	);

	reflection::Service<RenderService>("RenderService")
		.cli({"-rdl", "-rdboe", "--gapi" })
	;
}


/*LLGL::Extent2D scaleResolution(const LLGL::Extent2D& res, float scale)
{
	const float wScaled = static_cast<float>(res.width) * scale;
	const float hScaled = static_cast<float>(res.height) * scale;
	return LLGL::Extent2D
	{
		static_cast<std::uint32_t>(wScaled + 0.5f),
		static_cast<std::uint32_t>(hScaled + 0.5f)
	};
}


LLGL::Extent2D scaleResolutionForDisplay(const LLGL::Extent2D& res, const LLGL::Display* display)
{
	if (display != nullptr)
		return scaleResolution(res, display->GetScale());
	else
		return res;
}*/

} //-- unnamed.


RenderService::CommandList* RenderService::CommandListPool::requestCommandList()
{
	return nullptr;
}


void RenderService::CommandListPool::initialize()
{
	//-- ToDo:
}


void RenderService::CommandListPool::submit()
{
	//-- ToDo:
}


bool RenderService::initialize(const Engine::Config::RenderParams& params)
{
	auto& ws = service<WindowsService>();
	auto& cli = service<CLIService>().parser();

	//-- Select Graphics API.
	m_gapi = GraphicsAPI::Unknown;

	std::string stringGAPI;
	cli("--gapi", "dx12") >> stringGAPI;

	rttr::enumeration type = rttr::type::get<GraphicsAPI>().get_enumeration();
	rttr::variant var = type.name_to_value(stringGAPI);
	if (var.is_valid())
	{
		m_gapi = var.get_value<GraphicsAPI>();
	}

	switch (m_gapi)
	{
	case GraphicsAPI::DirectX12:
	{
		m_backend = std::make_unique<render::d3d12::Backend>();
		break;
	}
	default:
	{
		ENGINE_FAIL("Invalid Graphics API!");
		return false;
	}
	}

	render::IBackend::Desc desc;
	//-- FrameCount means both the maximum  number of frames that will be queued to the GPU at a time,
	//-- as well as the number of back buffers in the DXGI swap chain.
	//-- For the majority of applications, this is convenient and works well.
	//-- However, there will be certain cases where an application may want to queue up more frames than there are back buffers available.
	//-- It should be noted that excessive buffering of frames dependent on user input may result in noticeable latency in your app.
	desc.numBuffers = params.numBackBuffers;
	desc.hwnd = ws.mainWindow()->handle();
	auto [w, h] = ws.mainWindow()->size();
	desc.width = w;
	desc.height = h;
	if (cli["-rdl"])
	{
		desc.flags |= render::IBackend::Flags::DebugLayer;
	}
	if (cli["-rdboe"])
	{
		desc.flags |= render::IBackend::Flags::DebugBreakOnError;
	}

	bool initialized = m_backend->initialize(desc);

	m_commandListPool.initialize();

	return initialized;
}


void RenderService::release()
{
	m_backend->release();
	m_backend.reset();
}


void RenderService::postTick()
{
	m_backend->present();
}

} //-- engine.
