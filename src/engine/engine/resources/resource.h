#pragma once

namespace engine::resources
{

class IResource
{
public:
	enum class Status : uint8_t
	{
		Failed,
		Loading,
		Ready
	};

	virtual ~IResource() = default;

	Status status() const { return m_status; }
	bool ready() const { return status() == Status::Ready; }

	void setStatus(const Status status) { m_status = status; }

protected:
	Status m_status = Status::Failed;
};

} //-- engine::resources.
