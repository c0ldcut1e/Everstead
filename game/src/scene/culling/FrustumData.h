#pragma once

#include <glm/vec3.hpp>

struct FrustumPlane
{
    glm::dvec3 normal;
    double distance;
};

struct FrustumData
{
    static const int s_planeCount = 6;
    FrustumPlane planes[s_planeCount];
};
