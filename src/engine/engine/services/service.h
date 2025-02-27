#pragma once
#include <engine/export.h>

namespace engine
{

class ENGINE_API IService
{
public:
	virtual ~IService() = default;

	virtual bool initialize() = 0;

private:
};

} //-- engine.
