#include "EnchantedIceBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../particle/ParticleSpawnParams.h"

EnchantedIceBlock::EnchantedIceBlock(const std::string &texturePath) : Block(true, texturePath)
{
    SetSlipperiness(0.98f);
    SetLightEmission(11);
    SetLightColor(0x80, 0xE0, 0xFF);
}

void EnchantedIceBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    if (random->NextInt(0, 2) != 0)
    {
        return;
    }

    Direction *face = Direction::ById(random->NextInt(1, 6));
    Block *neighbor = Block::ById(level->GetBlockId(BlockPos(pos.x + face->dx, pos.y + face->dy, pos.z + face->dz)));
    if (neighbor && neighbor->IsSolid() && !neighbor->IsTransparent())
    {
        return;
    }

    double u = random->NextDouble();
    double v = random->NextDouble();
    glm::dvec3 position(pos.x + 0.5 + face->dx * 0.55, pos.y + 0.5 + face->dy * 0.55, pos.z + 0.5 + face->dz * 0.55);
    if (face->dx != 0)
    {
        position = glm::dvec3(position.x, pos.y + u, pos.z + v);
    }
    else if (face->dy != 0)
    {
        position = glm::dvec3(pos.x + u, position.y, pos.z + v);
    }
    else
    {
        position = glm::dvec3(pos.x + u, pos.y + v, position.z);
    }

    const double driftSpeed = 0.02;
    ParticleSpawnParams params;
    params.type        = eParticleType_Glow;
    params.position    = position;
    params.velocity    = glm::dvec3(face->dx * driftSpeed, face->dy * driftSpeed + 0.005, face->dz * driftSpeed);
    params.color       = 0xFF8CF0FF;
    params.hasVelocity = true;
    params.hasColor    = true;
    level->SpawnParticle(params);
}
