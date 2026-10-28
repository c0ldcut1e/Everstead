#include "MinecartItem.h"

#include <memory>
#include <utility>

#include "../../entity/item/Minecart.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../block/RailBlock.h"

MinecartItem::MinecartItem(int minecartType)
{
    m_minecartType = minecartType;
}

int MinecartItem::GetMinecartType() const
{
    return m_minecartType;
}

bool MinecartItem::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    (void) face;
    return RailBlock::IsRail(Block::ById(level->GetBlockId(pos)));
}

void MinecartItem::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    (void) face;
    (void) look;
    if (level->IsClientSide())
    {
        return;
    }

    int shape = ((RailBlock *) Block::ById(level->GetBlockId(pos)))->GetShape(level->GetBlockData(pos));
    std::unique_ptr<Minecart> minecart(new Minecart(level));
    minecart->SetMinecartType(m_minecartType);
    minecart->SetPosition(glm::dvec3(pos.x + 0.5, pos.y + 0.0625 + (shape >= 2 && shape <= 5 ? 0.5 : 0.0), pos.z + 0.5));
    level->AddEntity(std::move(minecart));
}
