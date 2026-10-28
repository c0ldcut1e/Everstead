#include "EchoSeedItem.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../block/Block.h"

bool EchoSeedItem::CanUseOn(Level *level, const BlockPos &pos, Direction *face) const
{
    (void) face;
    return Block::ById(level->GetBlockId(pos)) == Block::ByName("resonant_crystal");
}

void EchoSeedItem::UseOn(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &look) const
{
    (void) face;
    (void) look;
    level->SetBlock(pos, Block::ByName("rich_resonant_crystal"));
    level->RecordBlockChange(pos);
}

void EchoSeedItem::SpawnUseEffects(Level *level, const BlockPos &pos) const
{
    for (int i = 0; i < 20; i++)
    {
        ParticleSpawnParams params;
        params.type = eParticleType_RiftSpark;
        params.position =
                glm::dvec3(pos.x + level->GetRandom().NextDouble(), pos.y + level->GetRandom().NextDouble(), pos.z + level->GetRandom().NextDouble());
        params.velocity    = glm::dvec3(level->GetRandom().NextGaussian() * 0.05, level->GetRandom().NextGaussian() * 0.05,
                                        level->GetRandom().NextGaussian() * 0.05);
        params.hasVelocity = true;
        level->SpawnParticle(params);
    }

    level->PlaySound("echo_seed.infuse", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f, 0.9f + level->GetRandom().NextFloat() * 0.2f);
}
