#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Model.h"

#include <numbers>

#include "../../core/Logger.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Textures.h"
#include "../lighting/Lighting.h"

#include <glm/gtc/matrix_transform.hpp>

Model::Model()
{
    m_root = std::make_unique<ModelPart>("root");
}

Model::Model(const ModelDefinition &definition)
{
    m_root = std::make_unique<ModelPart>("root");

    Rebuild(definition);
}

Model *Model::Copy() const
{
    Model *model  = new Model();
    model->m_skin = m_skin;

    if (m_root)
    {
        model->m_root = ClonePartTree(m_root.get());
    }

    return model;
}

void Model::Rebuild(const ModelDefinition &definition)
{
    m_skin = definition.GetSkin();
    m_root = BuildPartTree(*definition.GetRoot());
}

const ModelPartSkin &Model::GetSkin() const
{
    return m_skin;
}

ModelPart *Model::GetRoot()
{
    return m_root.get();
}

ModelPart *Model::FindPart(const std::string &name)
{
    if (!m_root)
    {
        return nullptr;
    }

    if (m_root->GetName() == name)
    {
        return m_root.get();
    }

    std::vector<ModelPart *> stack;
    stack.push_back(m_root.get());

    while (!stack.empty())
    {
        ModelPart *part = stack.back();
        stack.pop_back();

        if (part->GetName() == name)
        {
            return part;
        }

        for (const std::pair<const std::string, std::unique_ptr<ModelPart>> &kv : part->GetChildren())
        {
            stack.push_back(kv.second.get());
        }
    }

    return nullptr;
}

void Model::Render(TextureRepository &textures, uint32_t color, const glm::dmat4 &matrix) const
{
    Render(textures, color, matrix, m_root.get());
}

void Model::Render(TextureRepository &textures, uint32_t color, const glm::dmat4 &matrix, const ModelPart *part) const
{
    std::shared_ptr<Texture> texture;
    if (m_skin.GetType() == eModelPartSkinType_Player)
    {
        texture = textures.GetPlayerSkin(m_skin.GetTexturePath());
    }
    else if (m_skin.GetType() == eModelPartSkinType_Cape)
    {
        texture = textures.GetSized(m_skin.GetTexturePath(), m_skin.GetTextureWidth(), m_skin.GetTextureHeight());
    }
    else
    {
        texture = textures.Get(m_skin.GetTexturePath());
    }

    if (!texture.get())
    {
        Logger::LogError("Model render texture missing");
        return;
    }

    Textures::Bind(texture->GetId());

    int textureWidth  = m_skin.GetTextureWidth();
    int textureHeight = m_skin.GetTextureHeight();

    if (!part)
    {
        Logger::LogError("Model render part missing");
        Textures::Unbind();
        return;
    }

    RenderPart(part, textureWidth, textureHeight, color, matrix);

    Textures::Unbind();
}

void Model::RenderPart(const ModelPart *part, int textureWidth, int textureHeight, uint32_t color, const glm::dmat4 &matrix) const
{
    GlStateManager::PushMatrix();

    glm::dmat4 partMatrix = matrix;
    partMatrix            = glm::translate(partMatrix, part->GetPosition());
    partMatrix            = glm::translate(partMatrix, part->GetPivot());
    partMatrix            = glm::rotate(partMatrix, part->GetRotation().z * (std::numbers::pi / 180.0), glm::dvec3(0.0, 0.0, 1.0));
    partMatrix            = glm::rotate(partMatrix, part->GetRotation().y * (std::numbers::pi / 180.0), glm::dvec3(0.0, 1.0, 0.0));
    partMatrix            = glm::rotate(partMatrix, part->GetRotation().x * (std::numbers::pi / 180.0), glm::dvec3(1.0, 0.0, 0.0));
    partMatrix            = glm::translate(partMatrix, glm::dvec3(-part->GetPivot().x, -part->GetPivot().y, -part->GetPivot().z));

    GlStateManager::Translatef((float) part->GetPosition().x, (float) part->GetPosition().y, (float) part->GetPosition().z);
    GlStateManager::Translatef((float) part->GetPivot().x, (float) part->GetPivot().y, (float) part->GetPivot().z);
    GlStateManager::Rotatef((float) part->GetRotation().z, 0.0f, 0.0f, 1.0f);
    GlStateManager::Rotatef((float) part->GetRotation().y, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef((float) part->GetRotation().x, 1.0f, 0.0f, 0.0f);
    GlStateManager::Translatef((float) -part->GetPivot().x, (float) -part->GetPivot().y, (float) -part->GetPivot().z);

    const std::vector<ModelPart::Vertex> &vertices = part->GetVertices(textureWidth, textureHeight);
    if (!vertices.empty())
    {
        bool lit               = Lighting::IsEnabled();
        uint32_t colors[6]     = {Lighting::ShadeColor(color, lit ? Lighting::GetDiffuse(partMatrix, 0.0f, 0.0f, 1.0f) : 0.8f),
                                  Lighting::ShadeColor(color, lit ? Lighting::GetDiffuse(partMatrix, 0.0f, 0.0f, -1.0f) : 0.8f),
                                  Lighting::ShadeColor(color, lit ? Lighting::GetDiffuse(partMatrix, -1.0f, 0.0f, 0.0f) : 0.6f),
                                  Lighting::ShadeColor(color, lit ? Lighting::GetDiffuse(partMatrix, 1.0f, 0.0f, 0.0f) : 0.6f),
                                  Lighting::ShadeColor(color, lit ? Lighting::GetDiffuse(partMatrix, 0.0f, 1.0f, 0.0f) : 1.0f),
                                  Lighting::ShadeColor(color, lit ? Lighting::GetDiffuse(partMatrix, 0.0f, -1.0f, 0.0f) : 0.5f)};
        BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
        builder->Begin(ePrimitiveType_Triangles);
        for (size_t i = 0; i < vertices.size(); i++)
        {
            const ModelPart::Vertex &vertex = vertices[i];
            builder->Color(colors[vertex.face]);
            builder->VertexUV(vertex.x, vertex.y, vertex.z, vertex.u, vertex.v);
        }

        builder->End();
    }

    for (const std::pair<const std::string, std::unique_ptr<ModelPart>> &kv : part->GetChildren())
    {
        RenderPart(kv.second.get(), textureWidth, textureHeight, color, partMatrix);
    }

    GlStateManager::PopMatrix();
}

std::unique_ptr<ModelPart> Model::BuildPartTree(const ModelPartDefinition &definition)
{
    std::unique_ptr<ModelPart> part = std::make_unique<ModelPart>(definition.GetName());
    part->SetPivot(definition.GetPivot());
    part->SetRotation(definition.GetRotation());
    part->SetPosition(definition.GetPosition());

    for (const ModelPartCubeDefinition &cubeDefinition : definition.GetCubes())
    {
        glm::dvec3 min = cubeDefinition.GetMin();
        glm::dvec3 max = cubeDefinition.GetMax();
        float inflate  = cubeDefinition.GetInflate();

        ModelPart::Cube cube;
        cube.min = glm::dvec3(min.x - inflate, min.y - inflate, min.z - inflate);
        cube.max = glm::dvec3(max.x + inflate, max.y + inflate, max.z + inflate);
        cube.uv  = cubeDefinition.GetUV();

        part->AddCube(cube);
    }

    for (const std::pair<const std::string, std::unique_ptr<ModelPartDefinition>> &kv : definition.GetChildren())
    {
        part->AddChild(BuildPartTree(*kv.second));
    }

    return part;
}

std::unique_ptr<ModelPart> Model::ClonePartTree(const ModelPart *part)
{
    std::unique_ptr<ModelPart> copy = std::make_unique<ModelPart>(part->GetName());

    copy->SetPivot(part->GetPivot());
    copy->SetRotation(part->GetRotation());
    copy->SetPosition(part->GetPosition());

    for (const ModelPart::Cube &cube : part->GetCubes())
    {
        copy->AddCube(cube);
    }

    for (const std::pair<const std::string, std::unique_ptr<ModelPart>> &kv : part->GetChildren())
    {
        copy->AddChild(ClonePartTree(kv.second.get()));
    }

    return copy;
}

#endif
