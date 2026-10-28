#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelPartDefinition.h"

ModelPartDefinition::ModelPartDefinition()
{
    m_name     = "";
    m_pivot    = {0.0, 0.0, 0.0};
    m_rotation = {0.0, 0.0, 0.0};
    m_position = {0.0, 0.0, 0.0};
}

ModelPartDefinition::ModelPartDefinition(const std::string &name)
{
    m_name     = name;
    m_pivot    = {0.0, 0.0, 0.0};
    m_rotation = {0.0, 0.0, 0.0};
    m_position = {0.0, 0.0, 0.0};
}

const std::string &ModelPartDefinition::GetName() const
{
    return m_name;
}

void ModelPartDefinition::SetPivot(const glm::dvec3 &pivot)
{
    m_pivot = pivot;
}

void ModelPartDefinition::SetRotation(const glm::dvec3 &rotationDegrees)
{
    m_rotation = rotationDegrees;
}

void ModelPartDefinition::SetPosition(const glm::dvec3 &position)
{
    m_position = position;
}

const glm::dvec3 &ModelPartDefinition::GetPivot() const
{
    return m_pivot;
}

const glm::dvec3 &ModelPartDefinition::GetRotation() const
{
    return m_rotation;
}

const glm::dvec3 &ModelPartDefinition::GetPosition() const
{
    return m_position;
}

void ModelPartDefinition::AddCube(const ModelPartCubeDefinition &cube)
{
    m_cubes.push_back(cube);
}

const std::vector<ModelPartCubeDefinition> &ModelPartDefinition::GetCubes() const
{
    return m_cubes;
}

ModelPartDefinition *ModelPartDefinition::AddChild(std::unique_ptr<ModelPartDefinition> child)
{
    ModelPartDefinition *ptr     = child.get();
    m_children[child->GetName()] = std::move(child);
    return ptr;
}

ModelPartDefinition *ModelPartDefinition::GetChild(const std::string &name)
{
    auto it = m_children.find(name);
    if (it == m_children.end())
    {
        return nullptr;
    }

    return it->second.get();
}

const std::unordered_map<std::string, std::unique_ptr<ModelPartDefinition>> &ModelPartDefinition::GetChildren() const
{
    return m_children;
}

#endif
