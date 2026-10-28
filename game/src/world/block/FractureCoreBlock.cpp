#include "FractureCoreBlock.h"

#include <algorithm>
#include <cmath>

#include "../Explosion.h"
#include "../Level.h"

FractureCoreBlock::FractureCoreBlock() : Block(true, "assets/texture/block/fracture_core.png")
{
    m_openProperty   = AddProperty("open", 2);
    m_passesProperty = AddProperty("passes", 3);
}

bool FractureCoreBlock::IsOpen(uint16_t data) const
{
    return GetPropertyValue(data, m_openProperty) != 0;
}

// We return -1 unless the core sits in a ring of three frames and one socket, otherwise the weakest frame's stage
int FractureCoreBlock::GetStability(Level *level, const BlockPos &pos) const
{
    uint32_t frames[]       = {Block::ByName("broken_fracture_frame")->GetId(), Block::ByName("fractured_fracture_frame")->GetId(),
                               Block::ByName("cracked_fracture_frame")->GetId(), Block::ByName("sealed_fracture_frame")->GetId()};
    uint32_t socket         = Block::ByName("resonant_socket")->GetId();
    const BlockPos sides[4] = {BlockPos(1, 0, 0), BlockPos(-1, 0, 0), BlockPos(0, 0, 1), BlockPos(0, 0, -1)};
    int stability           = 3;
    int sockets             = 0;
    for (int i = 0; i < 4; i++)
    {
        uint32_t id = level->GetBlockId(pos + sides[i]);
        if (id == socket)
        {
            sockets++;
            continue;
        }

        int stage = 0;
        while (stage < 4 && frames[stage] != id)
        {
            stage++;
        }

        if (stage == 4)
        {
            return -1;
        }

        stability = std::min(stability, stage);
    }

    return sockets == 1 ? stability : -1;
}

// We return -1 unless the position stands inside an open tear, otherwise the stability of its core
int FractureCoreBlock::GetTearStability(Level *level, const glm::dvec3 &position, BlockPos &core) const
{
    BlockPos feet((int) floor(position.x), (int) floor(position.y), (int) floor(position.z));
    if (std::abs(position.x - feet.x - 0.5) >= 0.25 || std::abs(position.z - feet.z - 0.5) >= 0.25)
    {
        return -1;
    }

    for (int below = 1; below <= 3; below++)
    {
        BlockPos pos(feet.x, feet.y - below, feet.z);
        if (level->GetBlockId(pos) == GetId() && IsOpen(level->GetBlockData(pos)) && position.y - pos.y < 3.4)
        {
            core = pos;
            return GetStability(level, pos);
        }
    }

    return -1;
}

glm::dvec3 FractureCoreBlock::GetTearCenter(const BlockPos &pos) const
{
    return glm::dvec3(pos.x + 0.5, pos.y + 2.4, pos.z + 0.5);
}

float FractureCoreBlock::GetTearSeed(const BlockPos &pos) const
{
    return (float) ((pos.x * 73 + pos.y * 19 + pos.z * 37) % 97);
}

bool FractureCoreBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool FractureCoreBlock::OnUse(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || GetStability(level, pos) < 0)
    {
        return true;
    }

    auto lastToggle = m_lastToggle.find(pos);
    if (lastToggle != m_lastToggle.end() && level->GetGameTime() - lastToggle->second < 60)
    {
        return true;
    }

    SetOpen(level, pos, !IsOpen(level->GetBlockData(pos)));
    return true;
}

void FractureCoreBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (GetStability(level, pos) >= 0)
    {
        SetOpen(level, pos, true);
    }
}

void FractureCoreBlock::OnBreak(Level *level, const BlockPos &pos)
{
    m_lastToggle.erase(pos);
    if (!m_collapseStages.contains(pos) && IsOpen(level->GetBlockData(pos)) && GetStability(level, pos) >= 0)
    {
        level->PlaySound("fracture.close", glm::dvec3(pos.x + 0.5, pos.y + 2.4, pos.z + 0.5), 1.0f, 1.0f);
    }
}

void FractureCoreBlock::OnPass(Level *level, const BlockPos &pos, Random &random)
{
    int passes = GetPropertyValue(level->GetBlockData(pos), m_passesProperty) + 1;
    bool wear  = passes == 3 || (passes == 2 && random.NextInt(0, 1) == 0);
    level->SetBlockData(pos, WithPropertyValue(level->GetBlockData(pos), m_passesProperty, wear ? 0 : passes));
    level->RecordBlockChange(pos);
    if (!wear)
    {
        return;
    }

    Block *frames[] = {Block::ByName("broken_fracture_frame"), Block::ByName("fractured_fracture_frame"), Block::ByName("cracked_fracture_frame"),
                       Block::ByName("sealed_fracture_frame")};
    const BlockPos sides[4] = {BlockPos(1, 0, 0), BlockPos(-1, 0, 0), BlockPos(0, 0, 1), BlockPos(0, 0, -1)};
    for (int i = 0; i < 4; i++)
    {
        uint32_t id = level->GetBlockId(pos + sides[i]);
        for (int stage = 1; stage < 4; stage++)
        {
            if (frames[stage]->GetId() == id)
            {
                level->SetBlock(pos + sides[i], frames[stage - 1]);
                level->RecordBlockChange(pos + sides[i]);
            }
        }
    }
}

// The stage delays line up with the two booms and the implosion in collapse.ogg
void FractureCoreBlock::Collapse(Level *level, const BlockPos &pos)
{
    if (!m_collapseStages.emplace(pos, 0).second)
    {
        return;
    }

    Explosion(level, GetTearCenter(pos), 18.0f).Charge();
    level->ScheduleBlockForTick(pos, 22, 0);
}

void FractureCoreBlock::Tick(Level *level, const BlockPos &pos)
{
    auto stage = m_collapseStages.find(pos);
    if (stage == m_collapseStages.end())
    {
        return;
    }

    if (stage->second == 0)
    {
        Explosion(level, GetTearCenter(pos), 9.0f).Collapse(pos);
        stage->second = 1;
        level->ScheduleBlockForTick(pos, 26, 0);
    }
    else if (stage->second == 1)
    {
        Explosion(level, GetTearCenter(pos), 18.0f).Implode();
        stage->second = 2;
        level->ScheduleBlockForTick(pos, 16, 0);
    }
    else
    {
        Explosion(level, GetTearCenter(pos), 18.0f).Detonate(pos);
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
        m_collapseStages.erase(pos);
    }
}

void FractureCoreBlock::SetOpen(Level *level, const BlockPos &pos, bool open)
{
    m_lastToggle[pos] = level->GetGameTime();
    level->SetBlockData(pos, WithPropertyValue(level->GetBlockData(pos), m_openProperty, open ? 1 : 0));
    level->RecordBlockChange(pos);
    level->PlaySound(open ? "fracture.open" : "fracture.close", glm::dvec3(pos.x + 0.5, pos.y + 2.4, pos.z + 0.5), 1.0f, 1.0f);
}
