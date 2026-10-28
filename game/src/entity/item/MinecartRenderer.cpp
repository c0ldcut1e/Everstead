#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MinecartRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../rendering/GlStateManager.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRenderer.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/ModelRegistry.h"
#include "Minecart.h"

#include <glm/gtc/matrix_transform.hpp>

MinecartRenderer::MinecartRenderer()
{
    m_shadowRadius = 0.5f;
}

void MinecartRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const Minecart *minecart = (const Minecart *) entity;
    glm::dvec3 pos           = minecart->GetRenderPosition(partialTicks);
    float yaw                = minecart->GetRenderYaw(partialTicks);
    float pitch              = 0.0f;
    glm::dvec3 railPos;
    if (minecart->GetPos(pos, &railPos))
    {
        glm::dvec3 front;
        glm::dvec3 back;
        if (!minecart->GetPosOffs(pos, 0.3, &front))
        {
            front = railPos;
        }

        if (!minecart->GetPosOffs(pos, -0.3, &back))
        {
            back = railPos;
        }

        pos            = glm::dvec3(railPos.x, (front.y + back.y) / 2.0, railPos.z);
        glm::dvec3 dir = back - front;
        if (glm::length(dir) != 0.0)
        {
            dir   = glm::normalize(dir);
            yaw   = (float) (atan2(dir.z, dir.x) * 180.0 / std::numbers::pi);
            pitch = (float) (atan(dir.y) * 73.0);
        }
    }

    glm::dmat4 matrix = glm::translate(glm::dmat4(1.0), glm::dvec3(pos.x, pos.y + 0.375, pos.z));
    matrix            = glm::rotate(matrix, -yaw * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    matrix            = glm::rotate(matrix, pitch * std::numbers::pi / 180.0, glm::dvec3(0.0, 0.0, 1.0));
    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x, pos.y + 0.375, pos.z);
    GlStateManager::Rotatef(-yaw, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(pitch, 0.0f, 0.0f, 1.0f);
    float hitTime = (float) minecart->GetTimeSinceHit() - partialTicks;
    float damage  = std::max(0.0f, minecart->GetDamage() - partialTicks);
    if (hitTime > 0.0f)
    {
        float wobble = sinf(hitTime) * hitTime * damage / 10.0f * (float) minecart->GetHurtDir();
        matrix       = glm::rotate(matrix, wobble * std::numbers::pi / 180.0, glm::dvec3(1.0, 0.0, 0.0));
        GlStateManager::Rotatef(wobble, 1.0f, 0.0f, 0.0f);
    }

    if (minecart->GetMinecartType() != Minecart::eMinecartType_Rideable)
    {
        const char *kBlocks[] = {"", "chest", "furnace", "tnt", "hopper"};
        Block *block          = Block::ByName(kBlocks[minecart->GetMinecartType()]);
        if (minecart->GetMinecartType() == Minecart::eMinecartType_Furnace && minecart->IsLit())
        {
            block = Block::ByName("lit_furnace");
        }

        GlStateManager::PushMatrix();
        GlStateManager::Scalef(0.75f, 0.75f, 0.75f);
        GlStateManager::Translatef(0.0f, minecart->GetMinecartType() == Minecart::eMinecartType_Hopper ? 1.0f / 16.0f : 6.0f / 16.0f, 0.0f);
        int fuse   = minecart->GetFuse();
        float life = fuse - partialTicks + 1.0f;
        if (minecart->GetMinecartType() == Minecart::eMinecartType_Tnt && fuse >= 0)
        {
            if (life < 10.0f)
            {
                float g = std::clamp(1.0f - life / 10.0f, 0.0f, 1.0f);
                g       = g * g * g * g;
                float s = 1.0f + g * 0.3f;
                GlStateManager::Scalef(s, s, s);
            }

            if (fuse / 5 % 2 == 0)
            {
                float brightness = std::clamp((1.0f - life / 100.0f) * 0.8f, 0.0f, 1.0f);
                GlStateManager::SetOverlayColor(((uint32_t) (brightness * 255.0f) << 24) | 0xFFFFFF);
            }
        }

        BlockRenderer::RenderBlockAsItem(block, lightColor);
        GlStateManager::SetOverlayColor(0);
        GlStateManager::PopMatrix();
    }

    const float scale = 1.0f / 16.0f;
    matrix            = glm::scale(matrix, glm::dvec3(scale, scale, scale));
    GlStateManager::Scalef(scale, scale, scale);
    ModelRegistry::GetInstance()
            ->GetModel("minecart")
            ->Render(*ModelRegistry::GetInstance()->GetTextures(), Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor, matrix);
    GlStateManager::PopMatrix();

    RenderFire(entity, partialTicks);
    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

#endif
