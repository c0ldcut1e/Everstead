#include "SaplingBlock.h"

#include <algorithm>
#include <memory>

#include "../../utils/Random.h"
#include "../Level.h"
#include "../chunk/Chunk.h"
#include "../generation/AcaciaTreeFeature.h"
#include "../generation/BirchTreeFeature.h"
#include "../generation/CherryTreeFeature.h"
#include "../generation/DarkOakTreeFeature.h"
#include "../generation/JungleTreeFeature.h"
#include "../generation/MegaSpruceTreeFeature.h"
#include "../generation/SpruceTreeFeature.h"
#include "../generation/TreeFeature.h"

SaplingBlock::SaplingBlock(const std::string &texturePath, eTreeType treeType) : Block(false, texturePath)
{
    m_treeType = treeType;
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetAABB(AABB(glm::dvec3(0.1, 0.0, 0.1), glm::dvec3(0.9, 0.8, 0.9)));
    SetNeedsSupportBelow(true);
    SetRandomTicking(true);
}

bool SaplingBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == Block::ByName("grass_block") || below == Block::ByName("dirt") || below == Block::ByName("coarse_dirt") ||
           below == Block::ByName("podzol");
}

void SaplingBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || level->GetRandom().NextInt(0, 6) != 0 || !level->IsAreaLoaded(pos, 8))
    {
        return;
    }

    Grow(level, pos);
}

bool SaplingBlock::IsSameSapling(Level *level, int x, int y, int z) const
{
    return Block::ById(level->GetBlockId(BlockPos(x, y, z))) == this;
}

bool SaplingBlock::FindTwoByTwo(Level *level, const BlockPos &pos, int *offsetX, int *offsetZ) const
{
    for (int dx = 0; dx >= -1; dx--)
    {
        for (int dz = 0; dz >= -1; dz--)
        {
            int x = pos.x + dx;
            int z = pos.z + dz;
            if (IsSameSapling(level, x, pos.y, z) && IsSameSapling(level, x + 1, pos.y, z) && IsSameSapling(level, x, pos.y, z + 1) &&
                IsSameSapling(level, x + 1, pos.y, z + 1))
            {
                *offsetX = dx;
                *offsetZ = dz;
                return true;
            }
        }
    }

    return false;
}

void SaplingBlock::Grow(Level *level, const BlockPos &pos) const
{
    // Tree features only write into a chunk, so we grow the tree in a scratch chunk centered on the sapling and copy back what changed
    const int minY = std::max(pos.y - 1, 0);
    const int maxY = std::min(pos.y + MegaSpruceTreeFeature::k_maxHeight, Chunk::s_sizeY - 1);
    const int half = Chunk::s_sizeX / 2;

    std::unique_ptr<Chunk> scratch = std::make_unique<Chunk>(ChunkPos(0, 0));
    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int y = minY; y <= maxY; y++)
            {
                BlockPos worldPos(pos.x + x - half, y, pos.z + z - half);
                scratch->SetBlockId(x, y, z, level->GetBlockId(worldPos));
                scratch->SetBlockData(x, y, z, level->GetBlockData(worldPos));
            }
        }
    }

    Random random(level->GetRandom().NextUInt64());
    Block *air  = Block::ByName("air");
    int offsetX = 0;
    int offsetZ = 0;
    bool grown  = false;
    if ((m_treeType == eTreeType_Spruce || m_treeType == eTreeType_Jungle || m_treeType == eTreeType_DarkOak) &&
        FindTwoByTwo(level, pos, &offsetX, &offsetZ))
    {
        int originX = half + offsetX;
        int originZ = half + offsetZ;
        for (int dx = 0; dx <= 1; dx++)
        {
            for (int dz = 0; dz <= 1; dz++)
            {
                scratch->SetBlock(originX + dx, pos.y, originZ + dz, air);
            }
        }

        if (m_treeType == eTreeType_DarkOak)
        {
            grown = DarkOakTreeFeature().Place(*scratch, random, originX, pos.y, originZ);
        }
        else if (m_treeType == eTreeType_Jungle)
        {
            grown = JungleTreeFeature().PlaceMega(*scratch, random, originX, pos.y, originZ);
        }
        else
        {
            grown = MegaSpruceTreeFeature().Place(*scratch, random, originX, pos.y, originZ, random.NextInt(0, 1) == 1);
        }
    }
    else if (m_treeType != eTreeType_DarkOak)
    {
        scratch->SetBlock(half, pos.y, half, air);
        if (m_treeType == eTreeType_Spruce)
        {
            SpruceTreeFeature().Place(*scratch, random, half, pos.y, half);
        }
        else if (m_treeType == eTreeType_Birch)
        {
            BirchTreeFeature().Place(*scratch, random, half, pos.y, half);
        }
        else if (m_treeType == eTreeType_Jungle)
        {
            JungleTreeFeature().Place(*scratch, random, half, pos.y, half);
        }
        else if (m_treeType == eTreeType_Acacia)
        {
            AcaciaTreeFeature().Place(*scratch, random, half, pos.y, half);
        }
        else if (m_treeType == eTreeType_Cherry)
        {
            CherryTreeFeature().Place(*scratch, random, half, pos.y, half);
        }
        else
        {
            TreeFeature().Place(*scratch, random, half, pos.y, half);
        }

        grown = Block::ById(scratch->GetBlockId(half, pos.y, half)) != air;
    }

    if (!grown)
    {
        return;
    }

    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            for (int y = minY; y <= maxY; y++)
            {
                BlockPos worldPos(pos.x + x - half, y, pos.z + z - half);
                uint32_t id   = scratch->GetBlockId(x, y, z);
                uint16_t data = scratch->GetBlockData(x, y, z);
                if (id != level->GetBlockId(worldPos) || data != level->GetBlockData(worldPos))
                {
                    level->SetBlock(worldPos, Block::ById(id), nullptr, data);
                    level->RecordBlockChange(worldPos);
                }
            }
        }
    }
}
