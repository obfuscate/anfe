#pragma once
#include <engine/export.h>
#include <engine/pch.h>

#include <engine/services/service.h>

namespace engine
{

class Engine final : public utils::NonCopyable
{
public:
	ENGINE_API Engine() = default;

	ENGINE_API bool initialize(const int argc, const char* const argv[]);
	ENGINE_API void run();

private:
	using ServicePtr = std::unique_ptr<IService>;

	std::vector<ServicePtr> m_services;
};

} //-- engine.
