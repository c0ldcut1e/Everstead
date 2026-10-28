#include "DirtPathBlock.h"

#include "../Level.h"
#include "../chunk/Chunk.h"

DirtPathBlock::DirtPathBlock(const std::string &sidePath, const std::string &topPath, const std::string &bottomPath) : Block(true, sidePath)
{
    SetTexturePath(Direction::up, topPath);
    SetTexturePath(Direction::down, bottomPath);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetAABB(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 15.0 / 16.0, 1.0)));
}

bool DirtPathBlock::IsCoveredBySolid(Level *level, const BlockPos &pos)
{
    if (pos.y + 1 >= Chunk::s_sizeY)
    {
        return false;
    }

    Block *above = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)));
    return above->IsSolid() && above->IsCollidable() && !above->IsLiquid();
}

void DirtPathBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide() && IsCoveredBySolid(level, pos))
    {
        level->SetBlock(pos, Block::ByName("dirt"));
        level->RecordBlockChange(pos);
    }
}
