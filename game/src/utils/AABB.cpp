#include "AABB.h"

#include <algorithm>

#include "../threading/ThreadStorage.h"

AABB::AABB()
{
    m_min = {0.0, 0.0, 0.0};
    m_max = {0.0, 0.0, 0.0};
}

AABB::AABB(const glm::dvec3 &min, const glm::dvec3 &max)
{
    m_min = min;
    m_max = max;
}

const glm::dvec3 &AABB::GetMin() const
{
    return m_min;
}

const glm::dvec3 &AABB::GetMax() const
{
    return m_max;
}

void AABB::Set(const glm::dvec3 &min, const glm::dvec3 &max)
{
    m_min = min;
    m_max = max;
}

AABB AABB::Translated(const glm::dvec3 &offset) const
{
    return AABB(m_min + offset, m_max + offset);
}

AABB AABB::Inflated(double amount) const
{
    glm::dvec3 offset(amount, amount, amount);
    return AABB(m_min - offset, m_max + offset);
}

bool AABB::Intersects(const AABB &other) const
{
    ThreadStorage::UseDefaultThreadStorage();

    if (m_max.x <= other.m_min.x || m_min.x >= other.m_max.x)
    {
        return false;
    }

    if (m_max.y <= other.m_min.y || m_min.y >= other.m_max.y)
    {
        return false;
    }

    if (m_max.z <= other.m_min.z || m_min.z >= other.m_max.z)
    {
        return false;
    }

    return true;
}

bool AABB::Contains(const glm::dvec3 &point) const
{
    ThreadStorage::UseDefaultThreadStorage();

    if (point.x < m_min.x || point.x > m_max.x)
    {
        return false;
    }

    if (point.y < m_min.y || point.y > m_max.y)
    {
        return false;
    }

    if (point.z < m_min.z || point.z > m_max.z)
    {
        return false;
    }

    return true;
}

bool AABB::Clip(const glm::dvec3 &origin, const glm::dvec3 &direction, double maxDistance, double *distance) const
{
    const double origins[]    = {origin.x, origin.y, origin.z};
    const double directions[] = {direction.x, direction.y, direction.z};
    const double mins[]       = {m_min.x, m_min.y, m_min.z};
    const double maxs[]       = {m_max.x, m_max.y, m_max.z};
    double tMin               = 0.0;
    double tMax               = maxDistance;
    for (int axis = 0; axis < 3; axis++)
    {
        if (directions[axis] == 0.0)
        {
            if (origins[axis] < mins[axis] || origins[axis] > maxs[axis])
            {
                return false;
            }

            continue;
        }

        double t0 = (mins[axis] - origins[axis]) / directions[axis];
        double t1 = (maxs[axis] - origins[axis]) / directions[axis];
        tMin      = std::max(tMin, std::min(t0, t1));
        tMax      = std::min(tMax, std::max(t0, t1));
        if (tMin > tMax)
        {
            return false;
        }
    }

    *distance = tMin;
    return true;
}
