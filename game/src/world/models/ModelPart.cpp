#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ModelPart.h"

ModelPart::ModelPart()
{
    m_name                = "";
    m_pivot               = {0.0, 0.0, 0.0};
    m_rotation            = {0.0, 0.0, 0.0};
    m_position            = {0.0, 0.0, 0.0};
    m_vertexTextureWidth  = 0;
    m_vertexTextureHeight = 0;
}

ModelPart::ModelPart(const std::string &name)
{
    m_name                = name;
    m_pivot               = {0.0, 0.0, 0.0};
    m_rotation            = {0.0, 0.0, 0.0};
    m_position            = {0.0, 0.0, 0.0};
    m_vertexTextureWidth  = 0;
    m_vertexTextureHeight = 0;
}

const std::string &ModelPart::GetName() const
{
    return m_name;
}

void ModelPart::SetPivot(const glm::dvec3 &pivot)
{
    m_pivot = pivot;
}

void ModelPart::SetRotation(const glm::dvec3 &rotationDegrees)
{
    m_rotation = rotationDegrees;
}

void ModelPart::SetPosition(const glm::dvec3 &position)
{
    m_position = position;
}

const glm::dvec3 &ModelPart::GetPivot() const
{
    return m_pivot;
}

const glm::dvec3 &ModelPart::GetRotation() const
{
    return m_rotation;
}

const glm::dvec3 &ModelPart::GetPosition() const
{
    return m_position;
}

void ModelPart::AddCube(const Cube &cube)
{
    m_cubes.push_back(cube);
    m_vertices.clear();
}

const std::vector<ModelPart::Cube> &ModelPart::GetCubes() const
{
    return m_cubes;
}

const std::vector<ModelPart::Vertex> &ModelPart::GetVertices(int textureWidth, int textureHeight) const
{
    if (!m_vertices.empty() && m_vertexTextureWidth == textureWidth && m_vertexTextureHeight == textureHeight)
    {
        return m_vertices;
    }

    m_vertices.clear();
    m_vertexTextureWidth  = textureWidth;
    m_vertexTextureHeight = textureHeight;
    for (const Cube &cube : m_cubes)
    {
        float x0 = cube.min.x;
        float y0 = cube.min.y;
        float z0 = cube.min.z;
        float x1 = cube.max.x;
        float y1 = cube.max.y;
        float z1 = cube.max.z;

        const ModelPartCubeDefinition::FaceUV &uv = cube.uv;
        float fu0                                 = (float) uv.front.x / textureWidth;
        float fv0                                 = (float) uv.front.y / textureHeight;
        float fu1                                 = (float) (uv.front.x + uv.front.width) / textureWidth;
        float fv1                                 = (float) (uv.front.y + uv.front.height) / textureHeight;
        float bu0                                 = (float) uv.back.x / textureWidth;
        float bv0                                 = (float) uv.back.y / textureHeight;
        float bu1                                 = (float) (uv.back.x + uv.back.width) / textureWidth;
        float bv1                                 = (float) (uv.back.y + uv.back.height) / textureHeight;
        float lu0                                 = (float) uv.left.x / textureWidth;
        float lv0                                 = (float) uv.left.y / textureHeight;
        float lu1                                 = (float) (uv.left.x + uv.left.width) / textureWidth;
        float lv1                                 = (float) (uv.left.y + uv.left.height) / textureHeight;
        float ru0                                 = (float) uv.right.x / textureWidth;
        float rv0                                 = (float) uv.right.y / textureHeight;
        float ru1                                 = (float) (uv.right.x + uv.right.width) / textureWidth;
        float rv1                                 = (float) (uv.right.y + uv.right.height) / textureHeight;
        float tu0                                 = (float) uv.top.x / textureWidth;
        float tv0                                 = (float) uv.top.y / textureHeight;
        float tu1                                 = (float) (uv.top.x + uv.top.width) / textureWidth;
        float tv1                                 = (float) (uv.top.y + uv.top.height) / textureHeight;
        float du0                                 = (float) uv.bottom.x / textureWidth;
        float dv0                                 = (float) uv.bottom.y / textureHeight;
        float du1                                 = (float) (uv.bottom.x + uv.bottom.width) / textureWidth;
        float dv1                                 = (float) (uv.bottom.y + uv.bottom.height) / textureHeight;

        m_vertices.insert(m_vertices.end(),
                          {{x0, y0, z1, fu0, fv1, 0}, {x1, y0, z1, fu1, fv1, 0}, {x1, y1, z1, fu1, fv0, 0}, {x0, y0, z1, fu0, fv1, 0},
                           {x1, y1, z1, fu1, fv0, 0}, {x0, y1, z1, fu0, fv0, 0}, {x1, y0, z0, bu0, bv1, 1}, {x0, y0, z0, bu1, bv1, 1},
                           {x0, y1, z0, bu1, bv0, 1}, {x1, y0, z0, bu0, bv1, 1}, {x0, y1, z0, bu1, bv0, 1}, {x1, y1, z0, bu0, bv0, 1},
                           {x0, y0, z0, lu0, lv1, 2}, {x0, y0, z1, lu1, lv1, 2}, {x0, y1, z1, lu1, lv0, 2}, {x0, y0, z0, lu0, lv1, 2},
                           {x0, y1, z1, lu1, lv0, 2}, {x0, y1, z0, lu0, lv0, 2}, {x1, y0, z1, ru0, rv1, 3}, {x1, y0, z0, ru1, rv1, 3},
                           {x1, y1, z0, ru1, rv0, 3}, {x1, y0, z1, ru0, rv1, 3}, {x1, y1, z0, ru1, rv0, 3}, {x1, y1, z1, ru0, rv0, 3},
                           {x0, y1, z1, tu0, tv1, 4}, {x1, y1, z1, tu1, tv1, 4}, {x1, y1, z0, tu1, tv0, 4}, {x0, y1, z1, tu0, tv1, 4},
                           {x1, y1, z0, tu1, tv0, 4}, {x0, y1, z0, tu0, tv0, 4}, {x0, y0, z0, du0, dv1, 5}, {x1, y0, z0, du1, dv1, 5},
                           {x1, y0, z1, du1, dv0, 5}, {x0, y0, z0, du0, dv1, 5}, {x1, y0, z1, du1, dv0, 5}, {x0, y0, z1, du0, dv0, 5}});
    }

    return m_vertices;
}

ModelPart *ModelPart::AddChild(std::unique_ptr<ModelPart> child)
{
    ModelPart *ptr               = child.get();
    m_children[child->GetName()] = std::move(child);
    return ptr;
}

ModelPart *ModelPart::GetChild(const std::string &name)
{
    auto it = m_children.find(name);
    if (it == m_children.end())
    {
        return nullptr;
    }

    return it->second.get();
}

const std::unordered_map<std::string, std::unique_ptr<ModelPart>> &ModelPart::GetChildren() const
{
    return m_children;
}

#endif
