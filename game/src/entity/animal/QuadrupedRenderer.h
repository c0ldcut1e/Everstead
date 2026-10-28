#pragma once

#include <string>

#include "../EntityRenderer.h"

#include <glm/mat4x4.hpp>

class Mob;
class Model;

class QuadrupedRenderer : public EntityRenderer
{
public:
    QuadrupedRenderer(const std::string &modelName, float shadowRadius, float babyHeadOffsetY, float babyHeadOffsetZ);

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;

protected:
    virtual void ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const;
    virtual uint32_t GetOverlayColor(const Mob *mob, float partialTicks) const;
    virtual void PoseModel(const Mob *mob, Model *model, float partialTicks) const;
    virtual void RenderLayers(const Mob *mob, uint32_t color, uint32_t lightColor, const glm::dmat4 &matrix, float partialTicks) const;
    virtual void RenderModel(const Mob *mob, Model *model, uint32_t color, const glm::dmat4 &matrix) const;

private:
    std::string m_modelName;
    float m_babyHeadOffsetY;
    float m_babyHeadOffsetZ;
};
