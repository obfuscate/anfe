#pragma once

#include <engine/assert.h>
#include <engine/utils/noncopyable.h>
#include <engine/utils/family.h>
#include <engine/reflection/common.h>

namespace engine
{

class IService : public utils::NonCopyable
{
public:
	using Type = uint8_t;

	IService(Type typeId) : m_typeId(typeId) { }
	virtual ~IService() = default;

	virtual bool initialize() = 0;
	virtual void release() {}
	virtual void tick() {}

	Type typeId() const { return m_typeId; }

private:
	Type m_typeId = 0;

public:
	RTTR_ENABLE()
};


template<typename T>
class Service : public IService
{
public:
	Service() : IService(fetchId()) {}

	static Type fetchId()
	{
		return utils::familyId<Type, IService, T>();
	}

	RTTR_ENABLE(IService)
};


class ServiceManager final
{
public:

	template<typename TService, typename... Args>
	bool add(Args&&... args)
	{
		static_assert(std::is_base_of_v<IService, TService>, "Your class has to be inherited from IService!");
		auto serviceId = TService::fetchId();
		if (m_services.size() <= serviceId)
		{
			m_services.resize(serviceId + 1);
		}

		m_services[serviceId] = std::make_unique<TService>(std::forward<Args>(args)...);
		return m_services[serviceId]->initialize();
	}

	template<typename TService>
	TService* find()
	{
		static_assert(std::is_base_of_v<IService, TService>, "Your class has to be inherited from IService!");

		auto serviceId = TService::fetchId(); //-- It uses mutex inside.
		ENGINE_ASSERT_DEBUG(serviceId < static_cast<IService::Type>(m_services.size()), "ServiceId is wrong!");
		return static_cast<TService*>(m_services[serviceId].get());
	}

	template<typename TService>
	TService& get()
	{
		TService* service = find<TService>();
		ENGINE_ASSERT_DEBUG(service != nullptr, "There isn't such service");
		return *service;
	}

	void tick();
	void release();

private:
	using ServicePtr = std::unique_ptr<IService>;
	std::vector<ServicePtr> m_services;
};

} //-- engine.
