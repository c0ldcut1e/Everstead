#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

class ModelPartCubeDefinition
{
public:
    struct UVRect
    {
        int x;
        int y;
        int width;
        int height;
    };

    struct FaceUV
    {
        UVRect left;
        UVRect right;
        UVRect front;
        UVRect back;
        UVRect top;
        UVRect bottom;
    };

    ModelPartCubeDefinition();
    ModelPartCubeDefinition(const std::string &name, const glm::dvec3 &min, const glm::dvec3 &max,
                            const FaceUV &uv, float inflate);

    const std::string &GetName() const;
    const glm::dvec3 &GetMin() const;
    const glm::dvec3 &GetMax() const;
    const FaceUV &GetUV() const;
    float GetInflate() const;

private:
    std::string m_name;
    glm::dvec3 m_min;
    glm::dvec3 m_max;
    FaceUV m_uv;
    float m_inflate;
};
