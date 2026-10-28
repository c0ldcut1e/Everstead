#include "LeavesBlock.h"

#include <algorithm>
#include <unordered_set>
#include <vector>

#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../chunk/Chunk.h"

LeavesBlock::LeavesBlock(const std::string &texturePath) : Block(true, texturePath)
{
    m_naturalProperty    = AddProperty("natural", 2);
    m_checkDecayProperty = AddProperty("check_decay", 2);
    SetBreakSound("dig.grass");
    SetTransparent(true);
    SetCullsSameNeighbor(false);
    SetLightOpacity(1);
    SetRandomTicking(true);
}

uint16_t LeavesBlock::GetNaturalData() const
{
    return WithPropertyValue(0, m_naturalProperty, 1);
}

bool LeavesBlock::IsLeaves(const Block *block)
{
    return dynamic_cast<const LeavesBlock *>(block) != nullptr;
}

bool LeavesBlock::IsLog(const Block *block)
{
    return block && block->GetResourceLocation().GetPath().ends_with("_log");
}

void LeavesBlock::FlagDecayAround(Level *level, const BlockPos &pos)
{
    // Leaves touching the tree only diagonally never get a neighbor update, so removing a log flags everything in range
    int radius = 4;
    for (int y = std::max(pos.y - radius, 0); y <= std::min(pos.y + radius, Chunk::s_sizeY - 1); y++)
    {
        for (int x = pos.x - radius; x <= pos.x + radius; x++)
        {
            for (int z = pos.z - radius; z <= pos.z + radius; z++)
            {
                BlockPos leaf(x, y, z);
                if (!level->HasChunk(ChunkPos(Mth::FloorDiv(x, Chunk::s_sizeX), Mth::FloorDiv(z, Chunk::s_sizeZ))))
                {
                    continue;
                }

                Block *block = Block::ById(level->GetBlockId(leaf));
                if (IsLeaves(block))
                {
                    block->OnNeighborChanged(level, leaf);
                }
            }
        }
    }
}

bool LeavesBlock::IsSupported(Level *level, const BlockPos &pos) const
{
    std::vector<BlockPos> frontier;
    std::vector<BlockPos> next;
    std::unordered_set<BlockPos> visited;
    frontier.push_back(pos);
    visited.insert(pos);

    for (int distance = 1; distance <= kSupportDistance && !frontier.empty(); distance++)
    {
        next.clear();
        for (size_t i = 0; i < frontier.size(); i++)
        {
            const BlockPos &current = frontier[i];
            BlockPos neighbors[]    = {BlockPos(current.x - 1, current.y, current.z), BlockPos(current.x + 1, current.y, current.z),
                                       BlockPos(current.x, current.y - 1, current.z), BlockPos(current.x, current.y + 1, current.z),
                                       BlockPos(current.x, current.y, current.z - 1), BlockPos(current.x, current.y, current.z + 1)};
            for (int j = 0; j < 6; j++)
            {
                const BlockPos &neighbor = neighbors[j];
                if (neighbor.y < 0 || neighbor.y >= Chunk::s_sizeY)
                {
                    continue;
                }

                if (!level->HasChunk(ChunkPos(Mth::FloorDiv(neighbor.x, Chunk::s_sizeX), Mth::FloorDiv(neighbor.z, Chunk::s_sizeZ))))
                {
                    return true;
                }

                if (!visited.insert(neighbor).second)
                {
                    continue;
                }

                Block *block = Block::ById(level->GetBlockId(neighbor));
                if (IsLog(block))
                {
                    return true;
                }

                if (IsLeaves(block))
                {
                    next.push_back(neighbor);
                }
            }
        }

        frontier.swap(next);
    }

    return false;
}

void LeavesBlock::RandomTick(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    if (GetPropertyValue(data, m_checkDecayProperty) == 0)
    {
        return;
    }

    if (IsSupported(level, pos))
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_checkDecayProperty, 0));
        level->RecordBlockChange(pos);
        return;
    }

    DropAsItem(level, pos, level->GetBlockData(pos));
    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

void LeavesBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    if (level->IsClientSide() || GetPropertyValue(data, m_naturalProperty) == 0 || GetPropertyValue(data, m_checkDecayProperty) == 1)
    {
        return;
    }

    level->SetBlockData(pos, WithPropertyValue(data, m_checkDecayProperty, 1));
    level->RecordBlockChange(pos);
}
