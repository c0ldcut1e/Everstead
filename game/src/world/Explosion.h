#pragma once

#include <vector>

#include "../utils/AABB.h"
#include "../utils/Random.h"
#include "block/BlockPos.h"

#include <glm/vec3.hpp>

class Level;

enum eEXPLOSION_TYPE
{
    eExplosionType_Normal            = 0,
    eExplosionType_RiftCharge        = 1,
    eExplosionType_RiftBurst         = 2,
    eExplosionType_RiftImplode       = 3,
    eExplosionType_RiftDetonate      = 4,
    eExplosionType_ExperimentLaunch  = 5,
    eExplosionType_ExperimentBurst   = 6,
    eExplosionType_ExperimentPull    = 7,
    eExplosionType_ExperimentFinale  = 8,
    eExplosionType_SingularityCharge = 9,
    eExplosionType_SingularityBlast  = 10,
    eExplosionType_NovaCharge        = 11,
    eExplosionType_NovaBlast         = 12,
    eExplosionType_TempestCharge     = 13,
    eExplosionType_TempestBlast      = 14,
    eExplosionType_PrismCharge       = 15,
    eExplosionType_PrismBlast        = 16,
    eExplosionType_HellgateCharge    = 17,
    eExplosionType_HellgateBlast     = 18,
};

class Explosion
{
public:
    Explosion(Level *level, const glm::dvec3 &center, float radius);

    void Explode();
    void Charge();
    void Collapse(const BlockPos &core);
    void Implode();
    void Detonate(const BlockPos &core);
    void Experiment(eEXPLOSION_TYPE type, const BlockPos &core);
    void SpawnParticles(const std::vector<BlockPos> &blocks);
    void SpawnChargeParticles();
    void SpawnBurstParticles();
    void SpawnImplodeParticles();
    void SpawnExperimentParticles(eEXPLOSION_TYPE type);

private:
    std::vector<BlockPos> CollectBlocks();
    void PushEntities(double strength, bool halfHeart, bool blast);
    void SendToPlayers(eEXPLOSION_TYPE type, const std::vector<BlockPos> &blocks);
    void DestroyBlocks(const std::vector<BlockPos> &blocks, float dropChance);
    float GetSeenPercent(const AABB &box);

    Level *m_level;
    glm::dvec3 m_center;
    float m_radius;
    Random m_random;
};
