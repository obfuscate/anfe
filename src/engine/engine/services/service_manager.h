#pragma once

#include <engine/assert.h>
#include <engine/utils/noncopyable.h>
#include <engine/utils/family.h>
#include <engine/utils/string.h>
#include <engine/reflection/common.h>
#include <engine/time.h>

namespace engine
{

namespace details
{

//-- because concept can't be inside a class...
template<typename T>
concept HasCreateCondition = requires
{
	{ T::createCondition() } -> std::same_as<bool>;
};

} //-- service::helpers.

class IService : public utils::NonCopyable
{
public:
	using Type = uint8_t;

	inline static constexpr auto kMetaCLI = "CLIArgs"_hs;

	IService(Type typeId) : m_typeId(typeId) { }
	virtual ~IService() = default;

	virtual void release() {}
	virtual void tick() {}
	virtual void postTick() {}

	Type typeId() const { return m_typeId; }

protected:
	Type m_typeId = 0;

	RTTR_ENABLE()
};


template<typename T>
class Service : public IService
{
public:
	Service() : IService(fetchId()) { }

	static Type fetchId()
	{
		return utils::familyId<Type, IService, T>();
	}

	RTTR_ENABLE(IService)
};


class ServiceManager final
{
public:

	template<typename T, typename... Args>
	bool add(Args&&... args)
	{
		static_assert(std::is_base_of_v<IService, T>, "Your class has to be inherited from IService!");
		if constexpr (details::HasCreateCondition<T>)
		{
			if (!T::createCondition()) //-- Also we may put in the meta reflection.
			{
				//-- Just don't create a service, but mark it as "succesfully" initialized.
				return true;
			}
		}

		auto serviceId = T::fetchId();
		if (m_services.size() <= serviceId)
		{
			m_services.resize(serviceId + 1);
		}

		m_services[serviceId] = std::make_unique<T>();
		T& service = *static_cast<T*>(m_services[serviceId].get());

		return service.initialize(std::forward<Args>(args)...);
	}

	template<typename TService>
	TService* find()
	{
		static_assert(std::is_base_of_v<IService, TService>, "Your class has to be inherited from IService!");

		auto serviceId = TService::fetchId(); //-- It uses mutex inside.
		if (serviceId < static_cast<IService::Type>(m_services.size()))
		{
			return static_cast<TService*>(m_services[serviceId].get());
		}
		else
		{
			return nullptr;
		}
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
