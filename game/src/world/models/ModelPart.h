#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ModelPartCubeDefinition.h"

#include <glm/vec3.hpp>

class ModelPart
{
public:
    struct Cube
    {
        glm::dvec3 min;
        glm::dvec3 max;
        ModelPartCubeDefinition::FaceUV uv;
    };

    struct Vertex
    {
        float x;
        float y;
        float z;
        float u;
        float v;
        int face;
    };

    ModelPart();
    explicit ModelPart(const std::string &name);

    const std::string &GetName() const;

    void SetPivot(const glm::dvec3 &pivot);
    void SetRotation(const glm::dvec3 &rotationDegrees);
    void SetPosition(const glm::dvec3 &position);

    const glm::dvec3 &GetPivot() const;
    const glm::dvec3 &GetRotation() const;
    const glm::dvec3 &GetPosition() const;

    void AddCube(const Cube &cube);
    const std::vector<Cube> &GetCubes() const;
    const std::vector<Vertex> &GetVertices(int textureWidth, int textureHeight) const;

    ModelPart *AddChild(std::unique_ptr<ModelPart> child);
    ModelPart *GetChild(const std::string &name);
    const std::unordered_map<std::string, std::unique_ptr<ModelPart>> &GetChildren() const;

private:
    std::string m_name;
    glm::dvec3 m_pivot;
    glm::dvec3 m_rotation;
    glm::dvec3 m_position;

    std::vector<Cube> m_cubes;
    // These are built on first render because UVs depend on the texture size of the model using this part
    mutable std::vector<Vertex> m_vertices;
    mutable int m_vertexTextureWidth;
    mutable int m_vertexTextureHeight;
    std::unordered_map<std::string, std::unique_ptr<ModelPart>> m_children;
};
