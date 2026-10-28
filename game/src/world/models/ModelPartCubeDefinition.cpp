#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelPartCubeDefinition.h"

ModelPartCubeDefinition::ModelPartCubeDefinition()
{
    m_name    = "";
    m_min     = glm::dvec3(0.0, 0.0, 0.0);
    m_max     = glm::dvec3(0.0, 0.0, 0.0);
    m_uv      = {};
    m_inflate = 0.0f;
}

ModelPartCubeDefinition::ModelPartCubeDefinition(const std::string &name, const glm::dvec3 &min, const glm::dvec3 &max, const FaceUV &uv,
                                                 float inflate)
{
    m_name    = name;
    m_min     = min;
    m_max     = max;
    m_uv      = uv;
    m_inflate = inflate;
}

const std::string &ModelPartCubeDefinition::GetName() const
{
    return m_name;
}

const glm::dvec3 &ModelPartCubeDefinition::GetMin() const
{
    return m_min;
}

const glm::dvec3 &ModelPartCubeDefinition::GetMax() const
{
    return m_max;
}

const ModelPartCubeDefinition::FaceUV &ModelPartCubeDefinition::GetUV() const
{
    return m_uv;
}

float ModelPartCubeDefinition::GetInflate() const
{
    return m_inflate;
}

#endif
