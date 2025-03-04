#include <engine/services/render_service.h>
#include <engine/helpers.h>
#include <engine/reflection/registration.h>
#include <engine/services/cli_service.h>
#include <engine/services/windows_service.h>

namespace engine
{

namespace
{

RTTR_REGISTRATION
{
	rttr::registration::enumeration<GraphicsAPI>("GraphicsAPI")
	(
		rttr::value("dx12", GraphicsAPI::DirectX12),
		rttr::value("vk", GraphicsAPI::Vulkan)
	);

	reflection::Service<RenderService>("RenderService")
		.cli({"-rdl", "--gapi", "-rdboe"})
	;
}


LLGL::Extent2D scaleResolution(const LLGL::Extent2D& res, float scale)
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
}

} //-- unnamed.

bool RenderService::initialize()
{
	auto& cli = instance().serviceManager().get<CLIService>().parser();

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

	ENGINE_ASSERT(m_gapi != GraphicsAPI::Unknown, "Invalid Graphics API!");
	//-- Forward all log reports to the standard output stream for errors
	//LLGL::Log::RegisterCallbackStd();

	LLGL::RenderSystemDescriptor rendererDesc;
	rendererDesc.flags = 0;
	if (cli["-rdl"])
	{
		m_debugger.reset(new LLGL::RenderingDebugger);

		rendererDesc.debugger = m_debugger.get();
		rendererDesc.flags |= LLGL::RenderSystemFlags::DebugDevice;
	}
	if (cli["-rdboe"])
	{
		rendererDesc.flags |= LLGL::RenderSystemFlags::DebugBreakOnError;
	}

	switch (m_gapi)
	{
	case GraphicsAPI::DirectX12:
	{
		rendererDesc.moduleName = "Direct3D12";
		break;
	}
	case GraphicsAPI::Vulkan:
	{
		rendererDesc.moduleName = "Vulkan";
		break;
	}
	default:
	{
		ENGINE_FAIL("Incorrect Graphics API");
	}
	}

	LLGL::Report report;
	m_renderer = LLGL::RenderSystem::Load(rendererDesc, &report);
	if (m_renderer == nullptr)
	{
		ENGINE_FAIL(fmt::format("Couldn't create a renderer. Error: {}", report.GetText()));
	}

	// Create swap-chain
	LLGL::SwapChainDescriptor swapChainDesc;
	{
		swapChainDesc.debugName = "SwapChain";
		swapChainDesc.resolution = scaleResolutionForDisplay(LLGL::Extent2D(1024, 768), LLGL::Display::GetPrimary());
		//-- ToDo.
		swapChainDesc.samples = std::min<std::uint32_t>(1, m_renderer->GetRenderingCaps().limits.maxColorBufferSamples);
		swapChainDesc.resizable = true;
	}
	m_swapChain = m_renderer->CreateSwapChain(swapChainDesc);

	m_swapChain->SetVsyncInterval(0);

	//samples_ = swapChain->GetSamples();

	//m_renderer->GetNativeHandle();

	return true;
}


void RenderService::release()
{
	m_renderer.reset();
}

} //-- engine.
