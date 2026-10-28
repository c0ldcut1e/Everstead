#ifndef EVERSTEAD_DEDICATED_SERVER

#include "FrustumCuller.h"

#include <glm/gtc/type_ptr.hpp>

FrustumCuller::FrustumCuller()
{}

void FrustumCuller::ExtractPlanes(const glm::dmat4 &viewProjection)
{
    const double *matrix = glm::value_ptr(viewProjection);

    m_data.planes[0].normal.x = matrix[3] + matrix[0];
    m_data.planes[0].normal.y = matrix[7] + matrix[4];
    m_data.planes[0].normal.z = matrix[11] + matrix[8];
    m_data.planes[0].distance = matrix[15] + matrix[12];

    m_data.planes[1].normal.x = matrix[3] - matrix[0];
    m_data.planes[1].normal.y = matrix[7] - matrix[4];
    m_data.planes[1].normal.z = matrix[11] - matrix[8];
    m_data.planes[1].distance = matrix[15] - matrix[12];

    m_data.planes[2].normal.x = matrix[3] + matrix[1];
    m_data.planes[2].normal.y = matrix[7] + matrix[5];
    m_data.planes[2].normal.z = matrix[11] + matrix[9];
    m_data.planes[2].distance = matrix[15] + matrix[13];

    m_data.planes[3].normal.x = matrix[3] - matrix[1];
    m_data.planes[3].normal.y = matrix[7] - matrix[5];
    m_data.planes[3].normal.z = matrix[11] - matrix[9];
    m_data.planes[3].distance = matrix[15] - matrix[13];

    m_data.planes[4].normal.x = matrix[3] + matrix[2];
    m_data.planes[4].normal.y = matrix[7] + matrix[6];
    m_data.planes[4].normal.z = matrix[11] + matrix[10];
    m_data.planes[4].distance = matrix[15] + matrix[14];

    m_data.planes[5].normal.x = matrix[3] - matrix[2];
    m_data.planes[5].normal.y = matrix[7] - matrix[6];
    m_data.planes[5].normal.z = matrix[11] - matrix[10];
    m_data.planes[5].distance = matrix[15] - matrix[14];

    for (int i = 0; i < FrustumData::s_planeCount; i++)
    {
        double length = glm::length(m_data.planes[i].normal);
        if (length > 0.0)
        {
            m_data.planes[i].normal.x /= length;
            m_data.planes[i].normal.y /= length;
            m_data.planes[i].normal.z /= length;
            m_data.planes[i].distance /= length;
        }
    }
}

const FrustumData &FrustumCuller::GetData() const
{
    return m_data;
}

bool FrustumCuller::TestAABB(const AABB &aabb) const
{
    const glm::dvec3 &min = aabb.GetMin();
    const glm::dvec3 &max = aabb.GetMax();

    for (int i = 0; i < FrustumData::s_planeCount; i++)
    {
        const FrustumPlane &plane = m_data.planes[i];

        glm::dvec3 positive(plane.normal.x >= 0.0 ? max.x : min.x, plane.normal.y >= 0.0 ? max.y : min.y, plane.normal.z >= 0.0 ? max.z : min.z);
        if (glm::dot(plane.normal, positive) + plane.distance < 0.0)
        {
            return false;
        }
    }

    return true;
}

bool FrustumCuller::TestSphere(const glm::dvec3 &center, double radius) const
{
    double safeRadius = radius < 0.0 ? 0.0 : radius;

    for (int i = 0; i < FrustumData::s_planeCount; i++)
    {
        const FrustumPlane &plane = m_data.planes[i];
        if (glm::dot(plane.normal, center) + plane.distance < -safeRadius)
        {
            return false;
        }
    }

    return true;
}

#endif
