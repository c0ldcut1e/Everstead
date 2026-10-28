#pragma once

#include <cstdint>
#include <vector>

#include "AcaciaTreeFeature.h"
#include "BirchTreeFeature.h"
#include "CherryTreeFeature.h"
#include "DarkOakTreeFeature.h"
#include "JungleTreeFeature.h"
#include "MegaSpruceTreeFeature.h"
#include "SpruceTreeFeature.h"
#include "SwampTreeFeature.h"
#include "TreeFeature.h"
#include "WorldGenStage.h"

class TerrainGenerator;

class TreeStage : public WorldGenStage
{
public:
    explicit TreeStage(TerrainGenerator *generator);

    void Apply(Chunk &chunk, const ChunkPos &pos) override;

private:
    struct TreeAttempt
    {
        ChunkPos owner;
        int x;
        int z;
        uint64_t seed;
    };

    void CollectTreeAttempts(const ChunkPos &owner, std::vector<TreeAttempt> *attempts) const;
    bool IsTreeCrowded(const TreeAttempt &attempt, const std::vector<TreeAttempt> &attempts) const;

    TerrainGenerator *m_generator;
    TreeFeature m_treeFeature;
    SpruceTreeFeature m_spruceTreeFeature;
    BirchTreeFeature m_birchTreeFeature;
    AcaciaTreeFeature m_acaciaTreeFeature;
    CherryTreeFeature m_cherryTreeFeature;
    SwampTreeFeature m_swampTreeFeature;
    JungleTreeFeature m_jungleTreeFeature;
    MegaSpruceTreeFeature m_megaSpruceTreeFeature;
    DarkOakTreeFeature m_darkOakTreeFeature;
};
