#include "BiomeRegistry.h"

#include "BiomeBadlands.h"
#include "BiomeBambooJungle.h"
#include "BiomeBeach.h"
#include "BiomeBirchForest.h"
#include "BiomeCave.h"
#include "BiomeCherryGrove.h"
#include "BiomeDarkForest.h"
#include "BiomeDesert.h"
#include "BiomeDesertCaves.h"
#include "BiomeDripstoneCaves.h"
#include "BiomeForest.h"
#include "BiomeFrozenRiver.h"
#include "BiomeGlacier.h"
#include "BiomeIceCaves.h"
#include "BiomeIceSpikes.h"
#include "BiomeJungle.h"
#include "BiomeJungleEdge.h"
#include "BiomeMagmaDepths.h"
#include "BiomeMeadow.h"
#include "BiomeMountains.h"
#include "BiomeOcean.h"
#include "BiomeOldGrowthSpruceTaiga.h"
#include "BiomeOvergrownHollows.h"
#include "BiomePlains.h"
#include "BiomeRiver.h"
#include "BiomeSavanna.h"
#include "BiomeSnowyTundra.h"
#include "BiomeSpruceForest.h"
#include "BiomeStonyShore.h"
#include "BiomeSwamp.h"
#include "BiomeWoodedBadlands.h"

BiomeRegistry *BiomeRegistry::GetInstance()
{
    static BiomeRegistry instance;
    return &instance;
}

void BiomeRegistry::Init()
{
    BiomeRegistry *registry = GetInstance();
    registry->Register("ocean", new BiomeOcean());
    registry->Register("plains", new BiomePlains());
    registry->Register("desert", new BiomeDesert());
    registry->Register("beach", new BiomeBeach());
    registry->Register("forest", new BiomeForest());
    registry->Register("spruce_forest", new BiomeSpruceForest());
    registry->Register("birch_forest", new BiomeBirchForest());
    registry->Register("savanna", new BiomeSavanna());
    registry->Register("snowy_tundra", new BiomeSnowyTundra());
    registry->Register("cave", new BiomeCave());
    registry->Register("mountains", new BiomeMountains());
    registry->Register("dripstone_caves", new BiomeDripstoneCaves());
    registry->Register("ice_caves", new BiomeIceCaves());
    registry->Register("desert_caves", new BiomeDesertCaves());
    registry->Register("river", new BiomeRiver());
    registry->Register("frozen_river", new BiomeFrozenRiver());
    registry->Register("ice_spikes", new BiomeIceSpikes());
    registry->Register("meadow", new BiomeMeadow());
    registry->Register("swamp", new BiomeSwamp());
    registry->Register("jungle", new BiomeJungle());
    registry->Register("jungle_edge", new BiomeJungleEdge());
    registry->Register("bamboo_jungle", new BiomeBambooJungle());
    registry->Register("overgrown_hollows", new BiomeOvergrownHollows());
    registry->Register("old_growth_spruce_taiga", new BiomeOldGrowthSpruceTaiga());
    registry->Register("glacier", new BiomeGlacier());
    registry->Register("cherry_grove", new BiomeCherryGrove());
    registry->Register("badlands", new BiomeBadlands());
    registry->Register("dark_forest", new BiomeDarkForest());
    registry->Register("stony_shore", new BiomeStonyShore());
    registry->Register("magma_depths", new BiomeMagmaDepths());
    registry->Register("wooded_badlands", new BiomeWoodedBadlands());
}
