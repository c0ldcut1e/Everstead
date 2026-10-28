#include "RedstoneTorchBlock.h"

#include "../Level.h"
#include "../particle/ParticleSpawnParams.h"

std::vector<RedstoneTorchBlock::Toggle> RedstoneTorchBlock::s_toggles;

RedstoneTorchBlock::RedstoneTorchBlock(bool lit, bool wall)
    : Block(false, lit ? "assets/texture/block/redstone_torch.png" : "assets/texture/block/redstone_torch_off.png")
{
    m_lit     = lit;
    m_wall    = wall;
    m_toggled = nullptr;
    SetBreakSound("dig.wood");
    SetRenderShape(eRenderShape_TORCH);
    SetTorchPlanes();
    SetNeedsAttachedSupport(true);
    SetRandomTicking(true);
    if (lit)
    {
        SetLightEmission(7);
        SetLightColor(0xFF, 0x40, 0x20);
    }

    float sideU0 = 7.0f / 16.0f;
    float sideU1 = 9.0f / 16.0f;
    float sideV0 = 6.0f / 16.0f;
    float sideV1 = 16.0f / 16.0f;
    SetUVRect(Direction::north, sideU0, sideV0, sideU1, sideV1);
    SetUVRect(Direction::south, sideU0, sideV0, sideU1, sideV1);
    SetUVRect(Direction::east, sideU0, sideV0, sideU1, sideV1);
    SetUVRect(Direction::west, sideU0, sideV0, sideU1, sideV1);
    SetUVRect(Direction::up, 7.0f / 16.0f, 6.0f / 16.0f, 9.0f / 16.0f, 8.0f / 16.0f);
    SetUVRect(Direction::down, 7.0f / 16.0f, 14.0f / 16.0f, 9.0f / 16.0f, 16.0f / 16.0f);
    if (wall)
    {
        SetAABB(AABB(glm::dvec3(0.4375f, 0.2f, 0.4375f), glm::dvec3(0.5625f, 0.825f, 0.5625f)));
        SetInteractionAABB(AABB(glm::dvec3(0.4f, 0.2f, 0.4f), glm::dvec3(0.6f, 0.8f, 0.6f)));
        SetInteractionAttachmentOffset(-0.12f);
        SetWallMountedTransform(22.5f, 0.475f);
        SetHorizontalFacing();
    }
    else
    {
        SetAABB(AABB(glm::dvec3(0.4375f, 0.0f, 0.4375f), glm::dvec3(0.5625f, 0.625f, 0.5625f)));
        SetNeedsCenterSupport(true);
    }
}

void RedstoneTorchBlock::SetToggled(RedstoneTorchBlock *toggled)
{
    m_toggled = toggled;
}

void RedstoneTorchBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (m_lit)
    {
        NotifySignalNeighbors(level, pos);
    }
}

void RedstoneTorchBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    (void) data;
    if (m_lit)
    {
        NotifySignalNeighbors(level, pos);
    }
}

void RedstoneTorchBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (!level->IsClientSide() && Block::ById(level->GetBlockId(pos)) == this)
    {
        level->ScheduleBlockForTick(pos, 2, 0);
    }
}

void RedstoneTorchBlock::Tick(Level *level, const BlockPos &pos)
{
    uint16_t data         = level->GetBlockData(pos);
    Direction *attachment = GetAttachment(data);
    bool powered =
            level->GetSignal(BlockPos(pos.x + attachment->dx, pos.y + attachment->dy, pos.z + attachment->dz), Direction::Opposite(attachment)) > 0;
    std::erase_if(s_toggles,
                  [level](const Toggle &toggle)
                  {
                      return toggle.level == level && level->GetGameTime() - toggle.time > 60;
                  });

    if (m_lit && powered)
    {
        level->SetBlock(pos, m_toggled, nullptr, data);
        level->RecordBlockChange(pos);
        if (IsToggledTooFrequently(level, pos, true))
        {
            Random &random = level->GetRandom();
            level->PlaySound("random.fizz", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.5f,
                             2.6f + (random.NextFloat() - random.NextFloat()) * 0.8f);
            for (int i = 0; i < 5; i++)
            {
                ParticleSpawnParams params;
                params.type     = eParticleType_Smoke;
                params.position = glm::dvec3(pos.x + random.NextDouble() * 0.6 + 0.2, pos.y + random.NextDouble() * 0.6 + 0.2,
                                             pos.z + random.NextDouble() * 0.6 + 0.2);
                level->SpawnParticle(params);
            }
        }
    }
    else if (!m_lit && !powered && !IsToggledTooFrequently(level, pos, false))
    {
        level->SetBlock(pos, m_toggled, nullptr, data);
        level->RecordBlockChange(pos);
    }
}

void RedstoneTorchBlock::RandomTick(Level *level, const BlockPos &pos)
{
    Tick(level, pos);
}

bool RedstoneTorchBlock::IsToggledTooFrequently(Level *level, const BlockPos &pos, bool add)
{
    if (add)
    {
        s_toggles.push_back(Toggle{level, pos, level->GetGameTime()});
    }

    int count = 0;
    for (size_t i = 0; i < s_toggles.size(); i++)
    {
        if (s_toggles[i].level == level && s_toggles[i].pos == pos)
        {
            count++;
        }
    }

    return count >= 8;
}

void RedstoneTorchBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    if (!m_lit)
    {
        return;
    }

    ParticleSpawnParams params;
    params.type     = eParticleType_Dust;
    params.position = glm::dvec3(pos.x + 0.5 + (random->NextFloat() - 0.5) * 0.2, pos.y + 0.7 + (random->NextFloat() - 0.5) * 0.2,
                                 pos.z + 0.5 + (random->NextFloat() - 0.5) * 0.2);
    if (m_wall)
    {
        Direction *attachment = GetAttachment(level->GetBlockData(pos));
        params.position += glm::dvec3(attachment->dx * 0.27, 0.22, attachment->dz * 0.27);
    }

    params.color    = 0xFFFF0000;
    params.hasColor = true;
    level->SpawnParticle(params);
}

int RedstoneTorchBlock::GetSignal(uint16_t data, Direction *direction) const
{
    return m_lit && direction != GetAttachment(data) ? 15 : 0;
}

int RedstoneTorchBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return direction == Direction::up ? GetSignal(data, direction) : 0;
}

bool RedstoneTorchBlock::IsSignalSource() const
{
    return true;
}

// The block above takes a direct signal, so blocks around every neighbor have to recheck
void RedstoneTorchBlock::NotifySignalNeighbors(Level *level, const BlockPos &pos)
{
    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *direction = Direction::ById(i);
        level->NotifyBlockNeighbors(BlockPos(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz));
    }
}
