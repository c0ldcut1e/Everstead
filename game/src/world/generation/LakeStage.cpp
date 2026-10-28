#include "LakeStage.h"

#include "../block/Block.h"
#include "TerrainGenerator.h"

LakeStage::LakeStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void LakeStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int surfaceLavaLakeChance = 10;

    const LakeFeature &lakeFeature = m_generator->GetLakeFeature();
    LakeFeature::Plan plan;
    m_generator->GetLakePlan(pos, &plan);
    Random random(m_generator->GetChunkSeed(pos, 0x2545f4914f6cdd1dULL));
    lakeFeature.Place(chunk, plan, Block::ByName("water_still"), random);

    if (!m_generator->GetSettings().lavaLakes || random.NextInt(0, m_generator->GetSettings().lavaLakeChance - 1) != 0)
    {
        return;
    }

    int y = random.NextInt(0, random.NextInt(0, TerrainGenerator::s_genDepth - 9) + 7);
    if (y >= TerrainGenerator::s_seaLevel && random.NextInt(0, surfaceLavaLakeChance - 1) != 0)
    {
        return;
    }

    Block *air = Block::ByName("air");
    while (y > 5 && Block::ById(chunk.GetBlockId(Chunk::s_sizeX / 2, y, Chunk::s_sizeZ / 2)) == air)
    {
        y--;
    }

    if (y <= 4)
    {
        return;
    }

    LakeFeature::Plan lavaPlan;
    lakeFeature.BuildPlan(random, y - LakeFeature::s_height / 2, &lavaPlan);
    lakeFeature.Place(chunk, lavaPlan, Block::ByName("lava_still"), random);
}
