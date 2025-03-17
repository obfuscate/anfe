#pragma once
#include <engine/utils/enum.h>

namespace engine::render
{

class IBackend
{
public:
	enum class Flags : uint8_t
	{
		None = 0,
		DebugLayer = 1 << 0,
		DebugBreakOnError = 1 << 1,
	};

	struct Desc
	{
		void* hwnd = nullptr;
		uint16_t width = 0;
		uint16_t height = 0;
		uint8_t numBuffers = 0;
		Flags flags = Flags::None;
	};

public:
	virtual ~IBackend() = default;

	virtual bool initialize(const Desc& desc) = 0;
	virtual void release() = 0;

	virtual void present() = 0;
};


DEFINE_ENUM_CLASS_BITWISE_OPERATORS(IBackend::Flags)

} //-- engine::render.
