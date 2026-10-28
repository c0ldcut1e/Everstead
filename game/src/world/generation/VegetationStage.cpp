#include "VegetationStage.h"

#include <algorithm>

#include "../biome/Biome.h"
#include "../block/Block.h"
#include "../block/CropBlock.h"
#include "../block/DoublePlantBlock.h"
#include "../block/LeavesBlock.h"
#include "../block/StemBlock.h"
#include "../block/SweetBerryBushBlock.h"
#include "TerrainGenerator.h"

VegetationStage::VegetationStage(TerrainGenerator *generator)
{
    m_generator = generator;
    m_flowerNoise.SetSeed((int) generator->GetSeed() ^ 0x5d17);
    m_flowerNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_flowerNoise.SetFrequency(1.0f / 48.0f);
    m_pumpkinNoise.SetSeed((int) generator->GetSeed() ^ 0x3a91);
    m_pumpkinNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_pumpkinNoise.SetFrequency(1.0f / 512.0f);
}

void VegetationStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    Block *grass      = Block::ByName("grass");
    Block *fern       = Block::ByName("fern");
    Block *grassBlock = Block::ByName("grass_block");
    Block *podzol     = Block::ByName("podzol");

    Random random(m_generator->GetChunkSeed(pos, 0x27d4eb2f165667c5ULL));

    Biome *centerBiome = chunk.GetSurfaceBiome(Chunk::s_sizeX / 2, Chunk::s_sizeZ / 2);
    int grassPatches   = centerBiome->GetGrassPatches();

    Block *air = Block::ByName("air");
    for (int patch = 0; patch < grassPatches; patch++)
    {
        int centerX = random.NextInt(0, Chunk::s_sizeX - 1);
        int centerZ = random.NextInt(0, Chunk::s_sizeZ - 1);
        int centerY = random.NextInt(0, std::max(TerrainGenerator::FindTopSolid(chunk, centerX, centerZ), 1) * 2);
        while (centerY > 0 && (Block::ById(chunk.GetBlockId(centerX, centerY, centerZ)) == air ||
                               LeavesBlock::IsLeaves(Block::ById(chunk.GetBlockId(centerX, centerY, centerZ)))))
        {
            centerY--;
        }

        Block *plant = centerBiome->HasFerns() && random.NextInt(0, 4) != 0 ? fern : grass;
        for (int attempt = 0; attempt < 128; attempt++)
        {
            int x = centerX + random.NextInt(0, 7) - random.NextInt(0, 7);
            int y = centerY + random.NextInt(0, 3) - random.NextInt(0, 3);
            int z = centerZ + random.NextInt(0, 7) - random.NextInt(0, 7);
            if (x >= 0 && x < Chunk::s_sizeX && z >= 0 && z < Chunk::s_sizeZ && y >= 1 && y < Chunk::s_sizeY &&
                Block::ById(chunk.GetBlockId(x, y, z)) == air &&
                (Block::ById(chunk.GetBlockId(x, y - 1, z)) == grassBlock || Block::ById(chunk.GetBlockId(x, y - 1, z)) == podzol) &&
                !chunk.GetSurfaceBiome(x, z)->IsSnowyAt(y))
            {
                chunk.SetBlock(x, y, z, plant);
            }
        }
    }

    const std::vector<std::string> &doublePlants = centerBiome->GetDoublePlants();
    if (!doublePlants.empty())
    {
        int minPatches = centerBiome->GetDoublePlantMin();
        int maxPatches = centerBiome->GetDoublePlantMax();
        int patches    = minPatches == maxPatches ? minPatches : minPatches + random.NextInt(0, maxPatches - minPatches);
        for (int patch = 0; patch < patches; patch++)
        {
            int index = doublePlants.size() > 1 ? random.NextInt(0, (int) doublePlants.size() - 1) : 0;
            PlaceDoublePlant(chunk, random, (DoublePlantBlock *) Block::ByName(doublePlants[index]), centerBiome);
        }
    }

    if (!centerBiome->GetNoiseFlowers().empty())
    {
        PlaceNoiseFlowers(chunk, random, centerBiome);
    }
    else
    {
        PlaceFlowers(chunk, random, centerBiome);
    }

    if (centerBiome->IsSnowy() || centerBiome == Biome::ByName("desert"))
    {
        return;
    }

    int centerTop       = TerrainGenerator::FindTopSolid(chunk, Chunk::s_sizeX / 2, Chunk::s_sizeZ / 2);
    float pumpkinRegion = m_pumpkinNoise.GetNoise((float) (chunk.GetPos().x * Chunk::s_sizeX), (float) (chunk.GetPos().z * Chunk::s_sizeZ));
    if (pumpkinRegion > 0.0f && random.NextInt(0, 71) == 0)
    {
        PlacePatch(chunk, random, Block::ByName("pumpkin_stem"), 3, 10, 1, Chunk::s_sizeY - 2);
    }

    if ((centerBiome == Biome::ByName("savanna") || centerBiome->HasMelons()) && random.NextInt(0, 47) == 0)
    {
        PlacePatch(chunk, random, Block::ByName("melon_stem"), 3, 10, 1, Chunk::s_sizeY - 2);
    }

    if (centerBiome == Biome::ByName("plains") && random.NextInt(0, 119) == 0)
    {
        PlacePatch(chunk, random, Block::ByName("carrots"), 2, 12, 1, Chunk::s_sizeY - 2);
    }

    if (centerTop >= TerrainGenerator::s_seaLevel - 1 && centerTop <= TerrainGenerator::s_seaLevel + 2 && random.NextInt(0, 71) == 0)
    {
        PlacePatch(chunk, random, Block::ByName("beetroots"), 2, 12, TerrainGenerator::s_seaLevel, TerrainGenerator::s_seaLevel + 3);
    }

    if ((centerBiome == Biome::ByName("mountains") || centerBiome == Biome::ByName("meadow")) && random.NextInt(0, 29) == 0)
    {
        PlacePatch(chunk, random, Block::ByName("potatoes"), 3, 20, 1, Chunk::s_sizeY - 2);
    }

    if (centerBiome == Biome::ByName("spruce_forest") && random.NextInt(0, 31) == 0)
    {
        PlacePatch(chunk, random, Block::ByName("sweet_berry_bush"), 7, 96, 1, Chunk::s_sizeY - 2);
    }
}

void VegetationStage::PlacePatch(Chunk &chunk, Random &random, Block *block, int spread, int attempts, int minY, int maxY) const
{
    Block *grassBlock         = Block::ByName("grass_block");
    Block *dirt               = Block::ByName("dirt");
    Block *air                = Block::ByName("air");
    CropBlock *crop           = dynamic_cast<CropBlock *>(block);
    StemBlock *stem           = dynamic_cast<StemBlock *>(block);
    SweetBerryBushBlock *bush = dynamic_cast<SweetBerryBushBlock *>(block);
    Direction *directions[]   = {Direction::north, Direction::south, Direction::west, Direction::east};
    int centerX               = random.NextInt(spread, Chunk::s_sizeX - 1 - spread);
    int centerZ               = random.NextInt(spread, Chunk::s_sizeZ - 1 - spread);
    Biome *biome              = chunk.GetSurfaceBiome(centerX, centerZ);
    for (int attempt = 0; attempt < attempts; attempt++)
    {
        int x = centerX + random.NextInt(-spread, spread);
        int z = centerZ + random.NextInt(-spread, spread);
        int y = TerrainGenerator::FindTopSolid(chunk, x, z) + 1;
        if (chunk.GetSurfaceBiome(x, z) != biome || y < minY || y > maxY || biome->IsSnowyAt(y))
        {
            continue;
        }

        Block *below = Block::ById(chunk.GetBlockId(x, y - 1, z));
        if ((below != grassBlock && below != dirt) || Block::ById(chunk.GetBlockId(x, y, z)) != air)
        {
            continue;
        }

        chunk.SetBlock(x, y, z, block);
        if (crop)
        {
            chunk.SetBlockData(x, y, z, crop->GetAgeData(random.NextInt(0, crop->GetMaxAge())));
        }
        else if (bush)
        {
            chunk.SetBlockData(x, y, z, bush->GetAgeData(SweetBerryBushBlock::kMaxAge));
        }
        else if (stem)
        {
            int age = random.NextInt(0, 1) == 0 ? StemBlock::kMaxAge : random.NextInt(0, StemBlock::kMaxAge - 1);
            chunk.SetBlockData(x, y, z, stem->WithAge(std::min(age, StemBlock::kMaxAge - 1)));
            Direction *direction = directions[random.NextInt(0, 3)];
            int fruitX           = x + direction->dx;
            int fruitZ           = z + direction->dz;
            if (age < StemBlock::kMaxAge || fruitX < 0 || fruitX >= Chunk::s_sizeX || fruitZ < 0 || fruitZ >= Chunk::s_sizeZ)
            {
                continue;
            }

            Block *fruitBelow = Block::ById(chunk.GetBlockId(fruitX, y - 1, fruitZ));
            if (Block::ById(chunk.GetBlockId(fruitX, y, fruitZ)) == air && (fruitBelow == grassBlock || fruitBelow == dirt))
            {
                chunk.SetBlock(fruitX, y, fruitZ, stem->GetFruit());
                chunk.SetBlock(x, y, z, stem->GetAttachedStem());
                chunk.SetBlockData(x, y, z, stem->GetAttachedStem()->GetPlacementData(direction));
            }
        }
    }
}

void VegetationStage::PlaceDoublePlant(Chunk &chunk, Random &random, DoublePlantBlock *plant, Biome *biome) const
{
    Block *air        = Block::ByName("air");
    Block *grassBlock = Block::ByName("grass_block");
    Block *podzol     = Block::ByName("podzol");
    int centerX       = random.NextInt(0, Chunk::s_sizeX - 1);
    int centerZ       = random.NextInt(0, Chunk::s_sizeZ - 1);
    int centerY       = random.NextInt(0, TerrainGenerator::FindTopSolid(chunk, centerX, centerZ) + 32);
    for (int attempt = 0; attempt < 64; attempt++)
    {
        int x = centerX + random.NextInt(0, 7) - random.NextInt(0, 7);
        int y = centerY + random.NextInt(0, 3) - random.NextInt(0, 3);
        int z = centerZ + random.NextInt(0, 7) - random.NextInt(0, 7);
        if (x < 0 || x >= Chunk::s_sizeX || z < 0 || z >= Chunk::s_sizeZ || y < 1 || y >= Chunk::s_sizeY - 1 || chunk.GetSurfaceBiome(x, z) != biome)
        {
            continue;
        }

        Block *ground = Block::ById(chunk.GetBlockId(x, y - 1, z));
        if ((ground == grassBlock || ground == podzol) && !biome->IsSnowyAt(y + 1) && Block::ById(chunk.GetBlockId(x, y, z)) == air &&
            Block::ById(chunk.GetBlockId(x, y + 1, z)) == air)
        {
            chunk.SetBlock(x, y, z, plant);
            chunk.SetBlock(x, y + 1, z, plant);
            chunk.SetBlockData(x, y + 1, z, plant->GetUpperData());
        }
    }
}

void VegetationStage::PlaceFlowers(Chunk &chunk, Random &random, Biome *biome) const
{
    const int patchSpread = 3;

    const std::vector<std::string> &flowers = biome->GetFlowers();
    if (flowers.empty())
    {
        return;
    }

    Block *grassBlock = Block::ByName("grass_block");
    Block *air        = Block::ByName("air");
    for (int patch = 0; patch < biome->GetFlowerPatches(); patch++)
    {
        if (random.NextInt(0, biome->GetFlowerPatchChance() - 1) != 0)
        {
            continue;
        }

        Block *flower = Block::ByName(flowers[random.NextInt(0, (int) flowers.size() - 1)]);
        int centerX   = random.NextInt(patchSpread, Chunk::s_sizeX - 1 - patchSpread);
        int centerZ   = random.NextInt(patchSpread, Chunk::s_sizeZ - 1 - patchSpread);
        for (int attempt = 0; attempt < biome->GetFlowerAttempts(); attempt++)
        {
            int x = centerX + random.NextInt(-patchSpread, patchSpread);
            int z = centerZ + random.NextInt(-patchSpread, patchSpread);

            // Patches stop at the biome border so flowers don't spill into biomes without them
            if (chunk.GetSurfaceBiome(x, z) != biome)
            {
                continue;
            }

            int y = TerrainGenerator::FindTopSolid(chunk, x, z);
            if (y >= 0 && y < Chunk::s_sizeY - 1 && Block::ById(chunk.GetBlockId(x, y, z)) == grassBlock && !biome->IsSnowyAt(y + 1) &&
                Block::ById(chunk.GetBlockId(x, y + 1, z)) == air)
            {
                chunk.SetBlock(x, y + 1, z, flower);
            }
        }
    }
}

void VegetationStage::PlaceNoiseFlowers(Chunk &chunk, Random &random, Biome *biome) const
{
    const std::vector<std::string> &flowers = biome->GetNoiseFlowers();
    int flowerCount                         = (int) flowers.size();

    Block *grassBlock = Block::ByName("grass_block");
    Block *air        = Block::ByName("air");
    Block *grass      = Block::ByName("grass");
    int baseX         = chunk.GetPos().x * Chunk::s_sizeX;
    int baseZ         = chunk.GetPos().z * Chunk::s_sizeZ;
    for (int attempt = 0; attempt < biome->GetNoiseFlowerAttempts(); attempt++)
    {
        int x = random.NextInt(0, Chunk::s_sizeX - 1);
        int z = random.NextInt(0, Chunk::s_sizeZ - 1);
        if (chunk.GetSurfaceBiome(x, z) != biome)
        {
            continue;
        }

        // Slow noise picks the species so each kind drifts across the biome in wide swaths
        float noise = m_flowerNoise.GetNoise((float) (baseX + x), (float) (baseZ + z));
        int index   = std::clamp((int) ((noise * 0.5f + 0.5f) * (float) flowerCount), 0, flowerCount - 1);
        int y       = TerrainGenerator::FindTopSolid(chunk, x, z);
        if (y >= 0 && y < Chunk::s_sizeY - 1 && Block::ById(chunk.GetBlockId(x, y, z)) == grassBlock && !biome->IsSnowyAt(y + 1))
        {
            Block *above = Block::ById(chunk.GetBlockId(x, y + 1, z));
            if (above == air || above == grass)
            {
                chunk.SetBlock(x, y + 1, z, Block::ByName(flowers[index]));
            }
        }
    }
}
