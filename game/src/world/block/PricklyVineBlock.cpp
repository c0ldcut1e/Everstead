#include "PricklyVineBlock.h"

#include "../Level.h"

PricklyVineBlock::PricklyVineBlock(const std::string &texturePrefix) : Block(false, texturePrefix + "_tip.png")
{
    m_tipProperty = AddProperty("tip", 2);
    m_variants[0] = AddTextureVariant(texturePrefix + ".png");
    m_variants[1] = AddTextureVariant(texturePrefix + "_tip.png");

    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetFlatItem(true);
    SetAABB(AABB(glm::dvec3(0.125, 0.0, 0.125), glm::dvec3(0.875, 1.0, 0.875)));
}

bool PricklyVineBlock::IsTip(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) != this;
}

Direction *PricklyVineBlock::GetAttachment(uint16_t data) const
{
    (void) data;
    return Direction::up;
}

bool PricklyVineBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    Block *above = Block::ById(level->GetBlockId(abovePos));
    if (above == this)
    {
        return true;
    }

    return above && above->IsFaceSturdy(level->GetBlockData(abovePos), Direction::down);
}

uint16_t PricklyVineBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos,
                                             const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;
    (void) look;

    return WithPropertyValue(0, m_tipProperty, IsTip(level, pos) ? 1 : 0);
}

void PricklyVineBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    if (!CanSurvive(level, pos, nullptr))
    {
        DropAsItem(level, pos, level->GetBlockData(pos));
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
        return;
    }

    uint16_t data    = level->GetBlockData(pos);
    uint16_t newData = WithPropertyValue(data, m_tipProperty, IsTip(level, pos) ? 1 : 0);
    if (newData == data)
    {
        return;
    }

    level->SetBlockData(pos, newData);
    level->RecordBlockChange(pos);
}

Block::UVRect PricklyVineBlock::GetCrossAtlasUVRect(uint16_t data) const
{
    return GetTextureVariantAtlasUVRect(GetCrossTextureVariant(data));
}

int PricklyVineBlock::GetCrossTextureVariant(uint16_t data) const
{
    return m_variants[GetPropertyValue(data, m_tipProperty)];
}

uint16_t PricklyVineBlock::MakeState(bool tip) const
{
    return WithPropertyValue(0, m_tipProperty, tip ? 1 : 0);
}
