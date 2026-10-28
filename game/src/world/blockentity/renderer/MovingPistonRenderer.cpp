#ifndef EVERSTEAD_DEDICATED_SERVER

#include "MovingPistonRenderer.h"

#include "../../../rendering/GlStateManager.h"
#include "../../block/BlockRenderer.h"
#include "../../block/PistonBaseBlock.h"
#include "../../block/PistonHeadBlock.h"
#include "../../lighting/Lighting.h"
#include "../PistonMovingBlockEntity.h"

// A retracting piston sits here as one piece, so we draw its base in place and only move the head
void MovingPistonRenderer::Render(const BlockEntity *blockEntity, float partialTicks) const
{
    PistonMovingBlockEntity *piston = (PistonMovingBlockEntity *) blockEntity;
    const BlockPos &pos             = piston->GetPos();
    Block *block                    = piston->GetBlock();
    uint16_t data                   = piston->GetData();
    glm::dvec3 offset               = piston->GetOffset(partialTicks);
    PistonBaseBlock *base           = dynamic_cast<PistonBaseBlock *>(block);
    if (piston->IsSource() && !piston->IsExtending() && base)
    {
        GlStateManager::PushMatrix();
        GlStateManager::Translated(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
        BlockRenderer::RenderBlockAsEntity(base, base->WithExtended(data, true), Lighting::GetEntityLight());
        GlStateManager::PopMatrix();

        PistonHeadBlock *head = (PistonHeadBlock *) Block::ByName("piston_head");
        block                 = head;
        data                  = head->WithSticky(head->WithFacing(0, base->GetFacing(data)), base->IsSticky());
    }

    if (block == Block::ByName("air"))
    {
        return;
    }

    GlStateManager::PushMatrix();
    GlStateManager::Translated(pos.x + 0.5 + offset.x, pos.y + 0.5 + offset.y, pos.z + 0.5 + offset.z);
    BlockRenderer::RenderBlockAsEntity(block, data, Lighting::GetEntityLight());
    GlStateManager::PopMatrix();
}

void MovingPistonRenderer::RenderItem(uint32_t color) const
{
    (void) color;
}

#endif
