#include "PistonMovingBlockEntity.h"

#include <algorithm>
#include <vector>

#include "../../entity/player/Player.h"
#include "../../tag/CompoundTag.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "../block/Block.h"

PistonMovingBlockEntity::PistonMovingBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos)
{
    m_block      = Block::ByName("air");
    m_data       = 0;
    m_facing     = Direction::up;
    m_extending  = true;
    m_source     = false;
    m_progress   = 0.0f;
    m_progressO  = 0.0f;
    m_lastTicked = 0;
}

const char *PistonMovingBlockEntity::GetId() const
{
    return "piston";
}

void PistonMovingBlockEntity::Tick()
{
    m_lastTicked = m_level->GetGameTime();
    m_progressO  = m_progress;
    if (m_progressO >= 1.0f)
    {
        if (!m_level->IsClientSide())
        {
            PlaceMovedBlock(m_block);
        }

        return;
    }

    m_progress = std::min(1.0f, m_progress + 0.5f);
    if (m_extending)
    {
        PushEntities(m_progress - m_progressO);
    }
}

// A moving block that already reached full progress is left alone so its own tick places it
void PistonMovingBlockEntity::Finish()
{
    if (m_progressO >= 1.0f)
    {
        return;
    }

    m_progress  = 1.0f;
    m_progressO = 1.0f;
    PlaceMovedBlock(m_source ? Block::ByName("air") : m_block);
}

// Setting the block deletes this block entity, so only locals are used after it
void PistonMovingBlockEntity::PlaceMovedBlock(Block *block)
{
    Level *level  = m_level;
    BlockPos pos  = m_pos;
    uint16_t data = block == m_block ? m_data : 0;
    if (Block::ById(level->GetBlockId(pos)) != Block::ByName("moving_piston"))
    {
        return;
    }

    level->SetBlock(pos, block, nullptr, data);
    level->RecordBlockChange(pos);
    Block::ById(level->GetBlockId(pos))->OnNeighborChanged(level, pos);
}

// The server moves every entity except players, which move themselves on their own client
void PistonMovingBlockEntity::PushEntities(double step)
{
    std::vector<AABB> boxes;
    m_block->GetCollisionBoxes(m_data, &boxes);
    glm::dvec3 direction(m_facing->dx, m_facing->dy, m_facing->dz);
    glm::dvec3 origin = glm::dvec3(m_pos.x, m_pos.y, m_pos.z) + GetOffset(1.0f);
    std::vector<Entity *> entities;
    if (m_level->IsClientSide())
    {
        entities.push_back(m_level->GetLocalPlayer());
    }
    else
    {
        const std::vector<std::unique_ptr<Entity>> &all = m_level->GetEntities();
        for (size_t i = 0; i < all.size(); i++)
        {
            if (!dynamic_cast<Player *>(all[i].get()))
            {
                entities.push_back(all[i].get());
            }
        }
    }

    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i];
        for (size_t j = 0; entity && j < boxes.size(); j++)
        {
            if (boxes[j].Translated(origin).Intersects(entity->GetAABB()))
            {
                entity->Move(direction.x * step, direction.y * step, direction.z * step);
                break;
            }
        }
    }
}

void PistonMovingBlockEntity::Init(Block *block, uint16_t data, Direction *facing, bool extending, bool source)
{
    m_block     = block;
    m_data      = data;
    m_facing    = facing;
    m_extending = extending;
    m_source    = source;
    m_progress  = 0.0f;
    m_progressO = 0.0f;
}

Block *PistonMovingBlockEntity::GetBlock() const
{
    return m_block;
}

uint16_t PistonMovingBlockEntity::GetData() const
{
    return m_data;
}

bool PistonMovingBlockEntity::IsSource() const
{
    return m_source;
}

bool PistonMovingBlockEntity::IsExtending() const
{
    return m_extending;
}

Direction *PistonMovingBlockEntity::GetFacing() const
{
    return m_facing;
}

float PistonMovingBlockEntity::GetProgress() const
{
    return m_progress;
}

int64_t PistonMovingBlockEntity::GetLastTicked() const
{
    return m_lastTicked;
}

glm::dvec3 PistonMovingBlockEntity::GetOffset(float partialTicks) const
{
    float progress = std::min(1.0f, m_progressO + (m_progress - m_progressO) * partialTicks);
    float distance = m_extending ? progress - 1.0f : 1.0f - progress;
    return glm::dvec3(m_facing->dx, m_facing->dy, m_facing->dz) * (double) distance;
}

void PistonMovingBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);
    tag->PutString("Block", m_block->GetResourceLocation().ToString());
    tag->PutInt("Data", m_data);
    tag->PutByte("Facing", (int8_t) m_facing->GetId());
    tag->PutBoolean("Extending", m_extending);
    tag->PutBoolean("Source", m_source);
    tag->PutFloat("Progress", m_progress);
}

void PistonMovingBlockEntity::Load(const CompoundTag *tag)
{
    m_block     = Block::ByName(tag->GetString("Block"));
    m_data      = (uint16_t) tag->GetInt("Data");
    m_facing    = Direction::ById((uint8_t) tag->GetByte("Facing"));
    m_extending = tag->GetBoolean("Extending");
    m_source    = tag->GetBoolean("Source");
    m_progress  = tag->GetFloat("Progress");
    m_progressO = m_progress;
}
