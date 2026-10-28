#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "ModelPartCubeDefinition.h"

#include <glm/vec3.hpp>

class ModelPartDefinition
{
public:
    ModelPartDefinition();
    explicit ModelPartDefinition(const std::string &name);

    const std::string &GetName() const;

    void SetPivot(const glm::dvec3 &pivot);
    void SetRotation(const glm::dvec3 &rotationDegrees);
    void SetPosition(const glm::dvec3 &position);

    const glm::dvec3 &GetPivot() const;
    const glm::dvec3 &GetRotation() const;
    const glm::dvec3 &GetPosition() const;

    void AddCube(const ModelPartCubeDefinition &cube);
    const std::vector<ModelPartCubeDefinition> &GetCubes() const;

    ModelPartDefinition *AddChild(std::unique_ptr<ModelPartDefinition> child);
    ModelPartDefinition *GetChild(const std::string &name);
    const std::unordered_map<std::string, std::unique_ptr<ModelPartDefinition>> &
    GetChildren() const;

private:
    std::string m_name;
    glm::dvec3 m_pivot;
    glm::dvec3 m_rotation;
    glm::dvec3 m_position;

    std::vector<ModelPartCubeDefinition> m_cubes;
    std::unordered_map<std::string, std::unique_ptr<ModelPartDefinition>> m_children;
};
