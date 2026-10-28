#include "LiquidBlock.h"

#include "../../utils/Random.h"
#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../chunk/Chunk.h"
#include "../particle/ParticleSpawnParams.h"

#include <glm/gtx/norm.hpp>

LiquidBlock::LiquidBlock(const std::string &texturePath, bool dynamic, bool lava) : Block(true, texturePath)
{
    m_dynamic     = dynamic;
    m_lava        = lava;
    m_counterpart = nullptr;
    SetTransparent(true);
    SetSelectable(false);
    SetCollidable(false);
    SetLiquid(true);
    SetReplaceable(true);
    AddProperty("level", 16);
    SetRandomTicking(lava && !dynamic);
}

void LiquidBlock::SetCounterpart(LiquidBlock *counterpart)
{
    m_counterpart = counterpart;
}

float LiquidBlock::GetHeight(uint8_t depth)
{
    if (depth >= 8)
    {
        return 8.0f / 9.0f;
    }

    return (float) (8 - depth) / 9.0f;
}

bool LiquidBlock::IsLava() const
{
    return m_lava;
}

int LiquidBlock::GetTickDelay() const
{
    const int waterTickDelay = 5;
    const int lavaTickDelay  = 30;
    return m_lava ? lavaTickDelay : waterTickDelay;
}

bool LiquidBlock::IsWaterAt(Level *level, const BlockPos &pos) const
{
    if (!HasLoadedBlock(level, pos))
    {
        return false;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    return block->IsLiquid() && !block->IsLava();
}

void LiquidBlock::Fizz(Level *level, const BlockPos &pos)
{
    Random &random = level->GetRandom();
    level->PlaySound("random.fizz", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.5f, 2.6f + (random.NextFloat() - random.NextFloat()) * 0.8f);
}

bool LiquidBlock::HardenLava(Level *level, const BlockPos &pos)
{
    if (!m_lava || Block::ById(level->GetBlockId(pos)) != this)
    {
        return false;
    }

    bool touchesWater = IsWaterAt(level, BlockPos(pos.x, pos.y, pos.z - 1)) || IsWaterAt(level, BlockPos(pos.x, pos.y, pos.z + 1)) ||
                        IsWaterAt(level, BlockPos(pos.x - 1, pos.y, pos.z)) || IsWaterAt(level, BlockPos(pos.x + 1, pos.y, pos.z)) ||
                        IsWaterAt(level, BlockPos(pos.x, pos.y + 1, pos.z));
    if (!touchesWater)
    {
        return false;
    }

    uint16_t depth = level->GetBlockData(pos);
    if (depth == 0)
    {
        Block *obsidian = Block::ByName("obsidian");
        level->SetBlock(pos, obsidian ? obsidian : Block::ByName("cobblestone"));
    }
    else
    {
        level->SetBlock(pos, Block::ByName("cobblestone"));
    }

    level->RecordBlockChange(pos);
    Fizz(level, pos);
    return true;
}

void LiquidBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (HardenLava(level, pos))
    {
        return;
    }

    if (m_dynamic)
    {
        level->ScheduleBlockForTick(pos, GetTickDelay(), 0);
    }
}

void LiquidBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (HardenLava(level, pos))
    {
        return;
    }

    if (!m_dynamic && Block::ById(level->GetBlockId(pos)) == this)
    {
        SetFlowing(level, pos, level->GetBlockData(pos));
    }
}

void LiquidBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    BlockPos ceiling(pos.x, pos.y - 1, pos.z);
    Block *ceilingBlock = Block::ById(level->GetBlockId(ceiling));
    if (random->NextInt(0, 9) != 0 || !ceilingBlock->IsFaceSturdy(level->GetBlockData(ceiling), Direction::up) ||
        Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 2, pos.z)))->IsCollidable())
    {
        return;
    }

    ParticleSpawnParams params;
    params.type     = m_lava ? eParticleType_DripLavaHang : eParticleType_DripWaterHang;
    params.position = glm::dvec3(pos.x + random->NextFloat(), pos.y - 1.05, pos.z + random->NextFloat());
    params.color    = m_lava ? 0xFFFF4915 : 0xFF334CFF;
    params.hasColor = true;
    level->SpawnParticle(params);
}

float LiquidBlock::GetLiquidHeight(uint8_t data) const
{
    return GetHeight(data);
}

glm::dvec3 LiquidBlock::GetFlow(Level *level, const BlockPos &pos) const
{
    glm::dvec3 flow;
    int middleDepth = GetRenderedDepth(level, pos);
    for (int direction = 0; direction < 4; direction++)
    {
        BlockPos neighbor = pos;
        if (direction == 0)
        {
            neighbor.x--;
        }
        else if (direction == 1)
        {
            neighbor.z--;
        }
        else if (direction == 2)
        {
            neighbor.x++;
        }
        else
        {
            neighbor.z++;
        }

        int depth = GetRenderedDepth(level, neighbor);
        if (depth < 0)
        {
            if (!IsLiquidBlocking(level, neighbor))
            {
                depth = GetRenderedDepth(level, BlockPos(neighbor.x, neighbor.y - 1, neighbor.z));
                if (depth >= 0)
                {
                    int difference = depth - (middleDepth - 8);
                    flow.x += (double) (neighbor.x - pos.x) * difference;
                    flow.z += (double) (neighbor.z - pos.z) * difference;
                }
            }
        }
        else
        {
            int difference = depth - middleDepth;
            flow.x += (double) (neighbor.x - pos.x) * difference;
            flow.z += (double) (neighbor.z - pos.z) * difference;
        }
    }

    if (level->GetBlockData(pos) >= 8 &&
        (IsLiquidBlocking(level, BlockPos(pos.x, pos.y, pos.z - 1)) || IsLiquidBlocking(level, BlockPos(pos.x, pos.y, pos.z + 1)) ||
         IsLiquidBlocking(level, BlockPos(pos.x - 1, pos.y, pos.z)) || IsLiquidBlocking(level, BlockPos(pos.x + 1, pos.y, pos.z)) ||
         IsLiquidBlocking(level, BlockPos(pos.x, pos.y + 1, pos.z - 1)) || IsLiquidBlocking(level, BlockPos(pos.x, pos.y + 1, pos.z + 1)) ||
         IsLiquidBlocking(level, BlockPos(pos.x - 1, pos.y + 1, pos.z)) || IsLiquidBlocking(level, BlockPos(pos.x + 1, pos.y + 1, pos.z))))
    {
        if (glm::length2(flow) > 0.0)
        {
            flow = glm::normalize(flow);
        }

        flow.y -= 6.0;
    }

    return glm::length2(flow) > 0.0 ? glm::normalize(flow) : flow;
}

void LiquidBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!m_dynamic)
    {
        return;
    }

    if (Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    if (HardenLava(level, pos))
    {
        return;
    }

    if (!HasLoadedBlock(level, BlockPos(pos.x - 1, pos.y, pos.z)) || !HasLoadedBlock(level, BlockPos(pos.x + 1, pos.y, pos.z)) ||
        !HasLoadedBlock(level, BlockPos(pos.x, pos.y, pos.z - 1)) || !HasLoadedBlock(level, BlockPos(pos.x, pos.y, pos.z + 1)))
    {
        level->ScheduleBlockForTick(pos, GetTickDelay(), 0);
        return;
    }

    int dropOff = m_lava ? 2 : 1;
    int depth   = GetDepth(level, pos);
    if (depth > 0)
    {
        int sourceCount  = 0;
        int highestDepth = GetHighestDepth(level, BlockPos(pos.x - 1, pos.y, pos.z), -100, &sourceCount);
        highestDepth     = GetHighestDepth(level, BlockPos(pos.x + 1, pos.y, pos.z), highestDepth, &sourceCount);
        highestDepth     = GetHighestDepth(level, BlockPos(pos.x, pos.y, pos.z - 1), highestDepth, &sourceCount);
        highestDepth     = GetHighestDepth(level, BlockPos(pos.x, pos.y, pos.z + 1), highestDepth, &sourceCount);

        int newDepth = highestDepth + dropOff;
        if (newDepth >= 8 || highestDepth < 0)
        {
            newDepth = -1;
        }

        int aboveDepth = GetDepth(level, BlockPos(pos.x, pos.y + 1, pos.z));
        if (aboveDepth >= 0)
        {
            newDepth = aboveDepth >= 8 ? aboveDepth : aboveDepth + 8;
        }

        BlockPos below(pos.x, pos.y - 1, pos.z);
        if (!m_lava && sourceCount >= 2 && level->GetGameRules().GetBool(eGameRule_WaterSourceConversion) &&
            (IsLiquidBlocking(level, below) || GetDepth(level, below) == 0))
        {
            newDepth = 0;
        }

        int tickDelay = GetTickDelay();
        if (m_lava && depth < 8 && newDepth < 8 && newDepth > depth && level->GetRandom().NextInt(0, 3) != 0)
        {
            tickDelay *= 4;
        }

        if (newDepth != depth)
        {
            depth = newDepth;
            if (depth < 0)
            {
                level->SetBlock(pos, Block::ByName("air"));
            }
            else
            {
                level->SetBlockData(pos, (uint8_t) depth);
                level->ScheduleBlockForTick(pos, tickDelay, 0);
                level->NotifyBlockNeighbors(pos);
            }
        }
        else
        {
            SetStill(level, pos);
        }
    }
    else
    {
        SetStill(level, pos);
    }

    BlockPos below(pos.x, pos.y - 1, pos.z);
    if (CanSpreadTo(level, below))
    {
        if (m_lava && IsWaterAt(level, below))
        {
            level->SetBlock(below, Block::ByName("stone"));
            level->RecordBlockChange(below);
            Fizz(level, below);
            return;
        }

        TrySpreadTo(level, below, (uint8_t) (depth >= 8 ? depth : depth + 8));
    }
    else if (depth >= 0 && (depth == 0 || IsLiquidBlocking(level, below)))
    {
        int neighborDepth = depth >= 8 ? 1 : depth + dropOff;
        if (neighborDepth < 8)
        {
            bool spread[4];
            GetSpread(level, pos, spread);
            if (spread[0])
            {
                TrySpreadTo(level, BlockPos(pos.x - 1, pos.y, pos.z), (uint8_t) neighborDepth);
            }

            if (spread[1])
            {
                TrySpreadTo(level, BlockPos(pos.x + 1, pos.y, pos.z), (uint8_t) neighborDepth);
            }

            if (spread[2])
            {
                TrySpreadTo(level, BlockPos(pos.x, pos.y, pos.z - 1), (uint8_t) neighborDepth);
            }

            if (spread[3])
            {
                TrySpreadTo(level, BlockPos(pos.x, pos.y, pos.z + 1), (uint8_t) neighborDepth);
            }
        }
    }
}

bool LiquidBlock::IsLiquidBlock(Block *block) const
{
    return block == this || block == m_counterpart;
}

int LiquidBlock::GetDepth(Level *level, const BlockPos &pos) const
{
    if (!HasLoadedBlock(level, pos))
    {
        return -1;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    if (!IsLiquidBlock(block))
    {
        return -1;
    }

    return level->GetBlockData(pos);
}

int LiquidBlock::GetRenderedDepth(Level *level, const BlockPos &pos) const
{
    int depth = GetDepth(level, pos);
    return depth >= 8 ? 0 : depth;
}

int LiquidBlock::GetHighestDepth(Level *level, const BlockPos &pos, int currentDepth, int *sourceCount) const
{
    int depth = GetDepth(level, pos);
    if (depth < 0)
    {
        return currentDepth;
    }

    if (depth == 0)
    {
        (*sourceCount)++;
    }

    if (depth >= 8)
    {
        depth = 0;
    }

    return currentDepth < 0 || depth < currentDepth ? depth : currentDepth;
}

bool LiquidBlock::HasLoadedBlock(Level *level, const BlockPos &pos) const
{
    if (pos.y < 0 || pos.y >= Chunk::s_sizeY)
    {
        return false;
    }

    return level->HasChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)));
}

bool LiquidBlock::IsLiquidBlocking(Level *level, const BlockPos &pos) const
{
    if (!HasLoadedBlock(level, pos))
    {
        return true;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    return (block->IsSolid() || block->BlocksLiquid()) && !block->IsLiquid() && !block->IsLiquidReplaceable();
}

bool LiquidBlock::CanSpreadTo(Level *level, const BlockPos &pos) const
{
    if (!HasLoadedBlock(level, pos))
    {
        return false;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    return !IsLiquidBlock(block) && !block->IsLava() && !block->BlocksLiquid() &&
           (!block->IsSolid() || block->IsLiquid() || block->IsLiquidReplaceable());
}

int LiquidBlock::GetSlopeDistance(Level *level, const BlockPos &pos, int pass, int from) const
{
    int lowest = 1000;
    for (int direction = 0; direction < 4; direction++)
    {
        if ((direction == 0 && from == 1) || (direction == 1 && from == 0) || (direction == 2 && from == 3) || (direction == 3 && from == 2))
        {
            continue;
        }

        BlockPos neighbor = pos;
        if (direction == 0)
        {
            neighbor.x--;
        }
        else if (direction == 1)
        {
            neighbor.x++;
        }
        else if (direction == 2)
        {
            neighbor.z--;
        }
        else
        {
            neighbor.z++;
        }

        if (IsLiquidBlocking(level, neighbor) || GetDepth(level, neighbor) == 0)
        {
            continue;
        }

        if (!IsLiquidBlocking(level, BlockPos(neighbor.x, neighbor.y - 1, neighbor.z)))
        {
            return pass;
        }

        if (pass < 4)
        {
            int distance = GetSlopeDistance(level, neighbor, pass + 1, direction);
            if (distance < lowest)
            {
                lowest = distance;
            }
        }
    }

    return lowest;
}

void LiquidBlock::GetSpread(Level *level, const BlockPos &pos, bool *outSpread) const
{
    int distances[4];
    int lowest = 1000;
    for (int direction = 0; direction < 4; direction++)
    {
        BlockPos neighbor = pos;
        if (direction == 0)
        {
            neighbor.x--;
        }
        else if (direction == 1)
        {
            neighbor.x++;
        }
        else if (direction == 2)
        {
            neighbor.z--;
        }
        else
        {
            neighbor.z++;
        }

        distances[direction] = 1000;
        if (!IsLiquidBlocking(level, neighbor) && GetDepth(level, neighbor) != 0)
        {
            BlockPos below(neighbor.x, neighbor.y - 1, neighbor.z);
            distances[direction] = IsLiquidBlocking(level, below) ? GetSlopeDistance(level, neighbor, 1, direction) : 0;
        }

        if (distances[direction] < lowest)
        {
            lowest = distances[direction];
        }
    }

    for (int direction = 0; direction < 4; direction++)
    {
        outSpread[direction] = distances[direction] == lowest;
    }
}

void LiquidBlock::SetFlowing(Level *level, const BlockPos &pos, uint8_t depth)
{
    LiquidBlock *flowingBlock = m_dynamic ? this : m_counterpart;
    level->SetBlockAndDataNoNeighborUpdates(pos, flowingBlock, depth);
    level->ScheduleBlockForTick(pos, GetTickDelay(), 0);
}

void LiquidBlock::SetStill(Level *level, const BlockPos &pos)
{
    level->SetBlockAndDataNoNeighborUpdates(pos, m_dynamic ? m_counterpart : this, level->GetBlockData(pos));
}

void LiquidBlock::TrySpreadTo(Level *level, const BlockPos &pos, uint8_t depth)
{
    if (CanSpreadTo(level, pos))
    {
        Block *replaced = Block::ById(level->GetBlockId(pos));
        if (m_lava && replaced != Block::ByName("air"))
        {
            Fizz(level, pos);
        }

        // Plants and cover that liquids wash over by the hundred would flood the level with drops
        if (!replaced->IsReplaceable() && !replaced->IsLiquidReplaceable())
        {
            replaced->DropAsItem(level, pos, level->GetBlockData(pos));
        }

        level->SetBlock(pos, m_dynamic ? this : m_counterpart, nullptr, depth);
    }
}

bool LiquidBlock::IsFlammableAt(Level *level, const BlockPos &pos) const
{
    return pos.y >= 0 && pos.y < Chunk::s_sizeY && Block::ById(level->GetBlockId(pos))->GetFlammability() > 0;
}

bool LiquidBlock::HasFlammableNeighbor(Level *level, const BlockPos &pos) const
{
    return IsFlammableAt(level, BlockPos(pos.x - 1, pos.y, pos.z)) || IsFlammableAt(level, BlockPos(pos.x + 1, pos.y, pos.z)) ||
           IsFlammableAt(level, BlockPos(pos.x, pos.y - 1, pos.z)) || IsFlammableAt(level, BlockPos(pos.x, pos.y + 1, pos.z)) ||
           IsFlammableAt(level, BlockPos(pos.x, pos.y, pos.z - 1)) || IsFlammableAt(level, BlockPos(pos.x, pos.y, pos.z + 1));
}

void LiquidBlock::RandomTick(Level *level, const BlockPos &pos)
{
    Random &random     = level->GetRandom();
    static Block *fire = Block::ByName("fire");

    if (!level->IsAreaLoaded(pos, 3))
    {
        return;
    }

    int steps = random.NextInt(0, 2);
    if (steps > 0)
    {
        BlockPos target = pos;
        for (int i = 0; i < steps; i++)
        {
            target = BlockPos(target.x + random.NextInt(0, 2) - 1, target.y + 1, target.z + random.NextInt(0, 2) - 1);
            if (target.y >= Chunk::s_sizeY)
            {
                return;
            }

            uint32_t blockId = level->GetBlockId(target);
            if (blockId == 0)
            {
                if (HasFlammableNeighbor(level, target))
                {
                    level->SetBlock(target, fire);
                    level->RecordBlockChange(target);
                    return;
                }
            }
            else if (Block::ById(blockId)->IsSolid())
            {
                return;
            }
        }

        return;
    }

    for (int i = 0; i < 3; i++)
    {
        BlockPos target(pos.x + random.NextInt(0, 2) - 1, pos.y, pos.z + random.NextInt(0, 2) - 1);
        BlockPos above(target.x, target.y + 1, target.z);
        if (above.y < Chunk::s_sizeY && level->GetBlockId(above) == 0 && IsFlammableAt(level, target))
        {
            level->SetBlock(above, fire);
            level->RecordBlockChange(above);
        }
    }
}
