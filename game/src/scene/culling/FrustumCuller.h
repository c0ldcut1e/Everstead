#pragma once

#include "Culler.h"
#include "FrustumData.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class FrustumCuller : public Culler
{
public:
    FrustumCuller();

    void ExtractPlanes(const glm::dmat4 &viewProjection);

    const FrustumData &GetData() const;

    bool TestAABB(const AABB &aabb) const override;
    bool TestSphere(const glm::dvec3 &center, double radius) const;

private:
    FrustumData m_data;
};
