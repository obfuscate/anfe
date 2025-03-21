#pragma once

#include <engine/math.h>

namespace engine::math
{

class AABB
{
public:
	AABB() : m_min(std::numeric_limits<float>::max()), m_max(std::numeric_limits<float>::lowest()) { }
	explicit AABB(const float min, const float max) : m_min(min), m_max(max) {}
	explicit AABB(const vec3& min, const vec3& max) : m_min(min), m_max(max) {}

	void extend(const vec3& pos)
	{
		m_min = min(m_min, pos);
		m_max = max(m_max, pos);
	}

	void extend(const AABB& aabb)
	{
		extend(aabb.m_min);
		extend(aabb.m_max);
	}

public:
	vec3 m_min;
	vec3 m_max;
};

} //-- engine::math.
