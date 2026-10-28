#pragma once

#include "../animal/QuadrupedRenderer.h"

class ZombieRenderer : public QuadrupedRenderer
{
public:
    explicit ZombieRenderer(const std::string &modelName = "zombie");

protected:
    void ApplyTransform(const Mob *mob, float partialTicks, glm::dmat4 *matrix) const override;
    void PoseModel(const Mob *mob, Model *model, float partialTicks) const override;
    void RenderModel(const Mob *mob, Model *model, uint32_t color, const glm::dmat4 &matrix) const override;
};
