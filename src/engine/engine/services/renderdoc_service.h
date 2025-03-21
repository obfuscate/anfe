#pragma once

#include <engine/services/service_manager.h>
#include <engine/integration/renderdoc/renderdoc_app.h>

namespace engine::render
{
//----------------------------------------------------------------------------------------------------------------------
class RenderDocService final : public Service<RenderDocService>
{
public:
	RenderDocService() = default;
	~RenderDocService() = default;

	bool initialize();
	void release() override;

	void setActiveWindow(void* device, void* wnd);
	void captureFrames(std::string_view path, const uint32_t numFrames);

public:
	static bool createCondition();

private:
	using BeginEventFn = void(*)(void*, uint64_t, const char*);
	using EndEventFn = void(*)(void*);

	RENDERDOC_API_1_6_0* m_api = nullptr;
	BeginEventFn m_beginEvent = nullptr;
	EndEventFn m_endEvent = nullptr;
	uint32_t m_pid = std::numeric_limits<uint32_t>::max();
};

} //-- engine::render.
