#pragma once
#include <engine/engine.h>
#include <engine/services/log_service.h>

namespace engine
{


template<typename T>
T& service()
{
	return engine().serviceManager().get<T>();
}


template<typename T>
T* findService()
{
	return engine().serviceManager().find<T>();
}


inline LogService& logger()
{
	return service<LogService>();
}


template<typename T>
inline void safeRelease(T*& pointer)
{
	if (pointer)
	{
		pointer->Release();
		pointer = nullptr;
	}
}

} //-- engine::helpers.
