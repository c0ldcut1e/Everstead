#include "RepeaterBlock.h"

#include "../Level.h"
#include "../particle/ParticleSpawnParams.h"

RepeaterBlock::RepeaterBlock() : DiodeBlock("assets/texture/block/repeater.png", "assets/texture/block/repeater_on.png")
{
    m_delayProperty  = AddProperty("delay", 4);
    m_lockedProperty = AddProperty("locked", 2);
    m_lockTexture    = AddTextureVariant("assets/texture/block/bedrock.png");
    AddTextureVariant("assets/texture/block/bedrock.png#rotated");
}

void RepeaterBlock::OnPlace(Level *level, const BlockPos &pos)
{
    DiodeBlock::OnPlace(level, pos);
    if (!level->IsClientSide())
    {
        CheckTickOnNeighbor(level, pos, level->GetBlockData(pos));
    }
}

void RepeaterBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    uint16_t data = level->GetBlockData(pos);
    if (!IsPowered(data))
    {
        return;
    }

    Direction *output = GetOutput(data);
    double x          = pos.x + 0.5 + (random->NextDouble() - 0.5) * 0.2;
    double y          = pos.y + 0.4 + (random->NextDouble() - 0.5) * 0.2;
    double z          = pos.z + 0.5 + (random->NextDouble() - 0.5) * 0.2;
    double offset     = -5.0;
    if (random->NextFloat() < 0.5f)
    {
        offset = (GetPropertyValue(data, m_delayProperty) + 1) * 2 - 1;
    }

    offset /= 16.0;
    ParticleSpawnParams params;
    params.type     = eParticleType_Dust;
    params.position = glm::dvec3(x - offset * output->dx, y, z - offset * output->dz);
    params.color    = 0xFFFF0000;
    params.hasColor = true;
    level->SpawnParticle(params);
}

bool RepeaterBlock::OnUse(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    SetData(level, pos, WithPropertyValue(data, m_delayProperty, (GetPropertyValue(data, m_delayProperty) + 1) % 4));
    return true;
}

bool RepeaterBlock::CanConnectRedstone(uint16_t data, Direction *direction) const
{
    Direction *output = GetOutput(data);
    return direction == output || direction == Direction::Opposite(output);
}

void RepeaterBlock::GetTorches(uint16_t data, std::vector<glm::dvec3> *out, std::vector<bool> *lit) const
{
    Direction *output = GetOutput(data);
    out->push_back(ToWorld(output, glm::dvec3(0.5, -3.0 / 16.0, 3.0 / 16.0)));
    lit->push_back(IsPowered(data));
    if (GetPropertyValue(data, m_lockedProperty) == 0)
    {
        out->push_back(ToWorld(output, glm::dvec3(0.5, -3.0 / 16.0, (7.0 + GetPropertyValue(data, m_delayProperty) * 2.0) / 16.0)));
        lit->push_back(IsPowered(data));
    }
}

void RepeaterBlock::GetElements(uint16_t data, std::vector<Element> *out) const
{
    AddBase(out, IsPowered(data));
    if (GetPropertyValue(data, m_lockedProperty) == 1)
    {
        float z = 6.0f + GetPropertyValue(data, m_delayProperty) * 2.0f;
        AddElement(out, glm::vec3(2.0f, 2.0f, z), glm::vec3(14.0f, 4.0f, z + 2.0f), m_lockTexture, glm::vec4(2.0f, 7.0f, 14.0f, 9.0f),
                   glm::vec4(6.0f, 7.0f, 8.0f, 9.0f), glm::vec4(7.0f, 2.0f, 9.0f, 14.0f), 90);
    }
}

void RepeaterBlock::CheckTickOnNeighbor(Level *level, const BlockPos &pos, uint16_t data)
{
    uint16_t locked = WithPropertyValue(data, m_lockedProperty, IsLocked(level, pos, data) ? 1 : 0);
    if (locked != data)
    {
        SetData(level, pos, locked);
    }

    DiodeBlock::CheckTickOnNeighbor(level, pos, locked);
}

bool RepeaterBlock::IsLocked(Level *level, const BlockPos &pos, uint16_t data) const
{
    return GetAlternateSignal(level, pos, data) > 0;
}

int RepeaterBlock::GetDelay(uint16_t data) const
{
    return (GetPropertyValue(data, m_delayProperty) + 1) * 2;
}

int RepeaterBlock::GetOutputSignal(uint16_t data) const
{
    (void) data;
    return 15;
}

bool RepeaterBlock::IsAlternateInput(Block *block) const
{
    return dynamic_cast<DiodeBlock *>(block);
}
