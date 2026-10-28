#include "FarmlandBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../chunk/Chunk.h"
#include "AttachedStemBlock.h"
#include "CropBlock.h"
#include "DirtPathBlock.h"
#include "StemBlock.h"

FarmlandBlock::FarmlandBlock(const std::string &texturePath, const std::string &dryTopPath, const std::string &wetTopPath) : Block(true, texturePath)
{
    m_moistureProperty = AddProperty("moisture", kMaxMoisture + 1);
    SetTexturePath(Direction::up, dryTopPath);
    SetSecondaryTexture(wetTopPath);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetRandomTicking(true);
    SetAABB(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 15.0 / 16.0, 1.0)));
}

void FarmlandBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 1.0, 1.0)));
}

Block::UVRect FarmlandBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;
    *flipU = false;
    if (direction == Direction::up && GetPropertyValue(data, m_moistureProperty) > 0)
    {
        return GetSecondaryAtlasUVRect();
    }

    return GetAtlasUVRect(direction);
}

bool FarmlandBlock::IsNearWater(Level *level, const BlockPos &pos)
{
    for (int x = pos.x - 4; x <= pos.x + 4; x++)
    {
        for (int y = pos.y; y <= pos.y + 1 && y < Chunk::s_sizeY; y++)
        {
            for (int z = pos.z - 4; z <= pos.z + 4; z++)
            {
                Block *block = Block::ById(level->GetBlockId(BlockPos(x, y, z)));
                if (block->IsLiquid() && !block->IsLava())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void FarmlandBlock::RandomTick(Level *level, const BlockPos &pos)
{
    int moisture = GetPropertyValue(level->GetBlockData(pos), m_moistureProperty);
    if (IsNearWater(level, pos) || level->IsRainingAt(BlockPos(pos.x, pos.y + 1, pos.z)))
    {
        if (moisture < kMaxMoisture)
        {
            level->SetBlockData(pos, WithPropertyValue(0, m_moistureProperty, kMaxMoisture));
            level->RecordBlockChange(pos);
        }

        return;
    }

    if (moisture > 0)
    {
        level->SetBlockData(pos, WithPropertyValue(0, m_moistureProperty, moisture - 1));
        level->RecordBlockChange(pos);
        return;
    }

    Block *above = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)));
    if (dynamic_cast<CropBlock *>(above) || dynamic_cast<StemBlock *>(above) || dynamic_cast<AttachedStemBlock *>(above))
    {
        return;
    }

    level->SetBlock(pos, Block::ByName("dirt"));
    level->RecordBlockChange(pos);
}

void FarmlandBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide() && DirtPathBlock::IsCoveredBySolid(level, pos))
    {
        level->SetBlock(pos, Block::ByName("dirt"));
        level->RecordBlockChange(pos);
    }
}

void FarmlandBlock::OnFallOn(Level *level, const BlockPos &pos, float fallDistance)
{
    if (level->GetRandom().NextFloat() < fallDistance - 0.5f)
    {
        BlockPos abovePos(pos.x, pos.y + 1, pos.z);
        Block *above = Block::ById(level->GetBlockId(abovePos));
        if (dynamic_cast<CropBlock *>(above) || dynamic_cast<StemBlock *>(above) || dynamic_cast<AttachedStemBlock *>(above))
        {
            above->DropAsItem(level, abovePos, level->GetBlockData(abovePos));
            level->SetBlock(abovePos, Block::ByName("air"));
            level->RecordBlockChange(abovePos);
        }

        level->SetBlock(pos, Block::ByName("dirt"));
        level->RecordBlockChange(pos);
    }
}
