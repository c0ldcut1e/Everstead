#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ItemEntityRenderer.h"

#include <cmath>

#include "../../core/Everstead.h"
#include "../../network/listener/ClientConnection.h"
#include "../../rendering/GlStateManager.h"
#include "../../scene/Camera.h"
#include "../../world/ItemInHandRenderer.h"
#include "../../world/block/Block.h"
#include "../../world/fracture/FractureTraits.h"
#include "../../world/item/Item.h"
#include "../player/MultiPlayerLocalPlayer.h"
#include "../player/RemotePlayer.h"
#include "ItemEntity.h"

static const float s_blockScale     = 0.25f;
static const float s_spriteScale    = 0.5f;
static const float s_bobHeight      = 0.1f;
static const float s_blockLift      = 0.125f;
static const float s_spinPerTick    = 1.0f / 20.0f;
static const float s_radToDeg       = 57.29578f;
static const float s_copySpread     = 0.15f;
static const float s_spriteSpacing  = 0.09375f;
static const double s_collectHeight = 0.5;

const Entity *ItemEntityRenderer::FindCollector(uint8_t playerIndex)
{
    MultiPlayerLocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    ClientConnection *connection        = localPlayer ? localPlayer->GetClientConnection() : nullptr;
    if (!connection)
    {
        return nullptr;
    }

    if (playerIndex == connection->GetPlayerIndex())
    {
        return localPlayer;
    }

    return connection->FindRemotePlayer(playerIndex);
}

static int GetRenderedCopies(int count)
{
    if (count > 48)
    {
        return 5;
    }

    if (count > 32)
    {
        return 4;
    }

    if (count > 16)
    {
        return 3;
    }

    if (count > 1)
    {
        return 2;
    }

    return 1;
}

void ItemEntityRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const ItemEntity *itemEntity = (const ItemEntity *) entity;
    const ItemInstance &item     = itemEntity->GetItem();
    if (item.IsEmpty())
    {
        return;
    }

    Item *type        = item.GetItem();
    Block *block      = type->GetBlock();
    bool renderAsCube = block && block->RendersAsItemBlock();
    bool flat         = !renderAsCube && Everstead::GetInstance()->options.flatItems;
    float scale       = renderAsCube ? s_blockScale : s_spriteScale;
    float time        = (float) itemEntity->GetAge() + partialTicks;
    float bob         = sinf(time / 10.0f + itemEntity->GetBobOffset()) * s_bobHeight + s_bobHeight;
    float spin        = (time * s_spinPerTick + itemEntity->GetBobOffset()) * s_radToDeg;
    int copies        = GetRenderedCopies(item.GetCount());

    glm::dvec3 position = itemEntity->GetRenderPosition(partialTicks);
    if (itemEntity->IsCollecting())
    {
        const Entity *collector = FindCollector(itemEntity->GetCollectorIndex());
        if (collector)
        {
            glm::dvec3 target = collector->GetRenderPosition(partialTicks) + glm::dvec3(0.0, s_collectHeight, 0.0);
            double t          = (double) itemEntity->GetCollectProgress(partialTicks);
            position          = glm::dvec3(position.x + (target.x - position.x) * t, position.y + (target.y - position.y) * t,
                                           position.z + (target.z - position.z) * t);
        }
    }

    GlStateManager::PushMatrix();
    GlStateManager::Translated(position.x, position.y + bob + scale * 0.5f + (renderAsCube ? s_blockLift : 0.0f), position.z);
    if (flat)
    {
        const glm::dvec3 &front = Everstead::GetInstance()->GetCamera()->GetFront();
        GlStateManager::Rotatef((float) atan2(front.x, front.z) * s_radToDeg, 0.0f, 1.0f, 0.0f);
    }
    else
    {
        GlStateManager::Rotatef(spin, 0.0f, 1.0f, 0.0f);
    }

    GlStateManager::Scalef(scale, scale, scale);

    // Offsets come from a fixed seed so the copies do not jitter between frames
    uint32_t seed = (uint32_t) itemEntity->GetId() * 2654435761u;
    GlStateManager::SetGlint(FractureTraits::GetGlint(item));
    for (int i = 0; i < copies; i++)
    {
        GlStateManager::PushMatrix();
        if (i > 0)
        {
            if (renderAsCube || flat)
            {
                float offsets[3];
                for (int axis = 0; axis < 3; axis++)
                {
                    seed          = seed * 1664525u + 1013904223u;
                    offsets[axis] = ((float) (seed >> 8) / 16777216.0f * 2.0f - 1.0f) * s_copySpread / scale;
                }

                GlStateManager::Translatef(offsets[0], offsets[1], offsets[2]);
            }
            else
            {
                GlStateManager::Translatef(0.0f, 0.0f, -s_spriteSpacing * (float) i);
            }
        }

        ItemInHandRenderer::RenderDroppedItem(type, lightColor, flat);
        GlStateManager::PopMatrix();
    }

    GlStateManager::SetGlint(0.0f);
    GlStateManager::PopMatrix();
    EntityRenderer::Render(entity, partialTicks, lightColor);
}

#endif
