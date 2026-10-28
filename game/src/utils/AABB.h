#pragma once

#include <glm/vec3.hpp>

class AABB
{
public:
    AABB();
    AABB(const glm::dvec3 &min, const glm::dvec3 &max);

    const glm::dvec3 &GetMin() const;
    const glm::dvec3 &GetMax() const;

    void Set(const glm::dvec3 &min, const glm::dvec3 &max);

    AABB Translated(const glm::dvec3 &offset) const;
    AABB Inflated(double amount) const;

    bool Intersects(const AABB &other) const;
    bool Contains(const glm::dvec3 &point) const;
    bool Clip(const glm::dvec3 &origin, const glm::dvec3 &direction, double maxDistance, double *distance) const;

private:
    glm::dvec3 m_min;
    glm::dvec3 m_max;
};
