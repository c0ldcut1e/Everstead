#pragma once

#include <memory>
#include <string>

#include "../../rendering/TextureRepository.h"
#include "ModelDefinition.h"
#include "ModelPart.h"
#include "ModelPartSkin.h"

#include <glm/mat4x4.hpp>

class Model
{
public:
    Model();
    explicit Model(const ModelDefinition &definition);

    Model *Copy() const;

    void Rebuild(const ModelDefinition &definition);

    const ModelPartSkin &GetSkin() const;

    ModelPart *GetRoot();
    ModelPart *FindPart(const std::string &name);

    void Render(TextureRepository &textures, uint32_t color, const glm::dmat4 &matrix) const;
    void Render(TextureRepository &textures, uint32_t color, const glm::dmat4 &matrix, const ModelPart *part) const;
    void RenderPart(const ModelPart *part, int textureWidth, int textureHeight, uint32_t color, const glm::dmat4 &matrix) const;

private:
    static std::unique_ptr<ModelPart> BuildPartTree(const ModelPartDefinition &definition);
    static std::unique_ptr<ModelPart> ClonePartTree(const ModelPart *part);

    ModelPartSkin m_skin;
    std::unique_ptr<ModelPart> m_root;
};
