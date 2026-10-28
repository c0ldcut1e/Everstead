#include "DoublePlantBlock.h"

#include "../Level.h"
#include "../chunk/Chunk.h"

DoublePlantBlock::DoublePlantBlock(const std::string &lowerTexturePath, const std::string &upperTexturePath) : Block(false, lowerTexturePath)
{
    m_halfProperty = AddProperty("half", 2);
    SetBreakSound("dig.grass");
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_Horizontal);
    SetAABB(AABB(glm::dvec3(0.1, 0.0, 0.1), glm::dvec3(0.9, 1.0, 0.9)));
    SetNeedsSupportBelow(true);
    SetSupportBlocks({"grass_block", "dirt", "coarse_dirt", "podzol"});
    SetFlammable(60, 100);
    SetSecondaryTexture(upperTexturePath);
}

bool DoublePlantBlock::IsUpper(uint16_t data) const
{
    return GetPropertyValue(data, m_halfProperty) != 0;
}

uint16_t DoublePlantBlock::GetUpperData() const
{
    return WithPropertyValue(0, m_halfProperty, 1);
}

void DoublePlantBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (IsUpper(level->GetBlockData(pos)))
    {
        return;
    }

    BlockPos above(pos.x, pos.y + 1, pos.z);
    level->SetBlock(above, this, nullptr, GetUpperData());
    level->RecordBlockChange(above);
}

void DoublePlantBlock::OnClientPlace(Level *level, const BlockPos &pos)
{
    OnPlace(level, pos);
}

void DoublePlantBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || CanSurvive(level, pos, nullptr))
    {
        return;
    }

    // Only a lower half that lost its ground drops, a half whose partner broke was already dropped by it
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (!IsUpper(level->GetBlockData(pos)) && Block::ById(level->GetBlockId(above)) == this)
    {
        DropAsItem(level, pos, 0);
    }

    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

BlockPos DoublePlantBlock::GetOtherPos(const BlockPos &pos, uint16_t data) const
{
    return BlockPos(pos.x, pos.y + (IsUpper(data) ? -1 : 1), pos.z);
}

bool DoublePlantBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    BlockPos below(pos.x, pos.y - 1, pos.z);
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (Block::ById(level->GetBlockId(pos)) == this)
    {
        if (IsUpper(level->GetBlockData(pos)))
        {
            return Block::ById(level->GetBlockId(below)) == this;
        }

        return Block::ById(level->GetBlockId(above)) == this && Block::CanSurvive(level, pos, attachment);
    }

    if (pos.y + 1 >= Chunk::s_sizeY || !Block::CanSurvive(level, pos, attachment))
    {
        return false;
    }

    Block *aboveBlock = Block::ById(level->GetBlockId(above));
    return !aboveBlock || aboveBlock == Block::ByName("air") || aboveBlock->CanBeReplaced(level->GetBlockData(above));
}

Block::UVRect DoublePlantBlock::GetCrossAtlasUVRect(uint16_t data) const
{
    return IsUpper(data) ? GetSecondaryAtlasUVRect() : GetAtlasUVRect(Direction::up);
}
