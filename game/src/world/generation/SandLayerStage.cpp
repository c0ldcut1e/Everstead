#include "SandLayerStage.h"

#include <algorithm>
#include <cmath>

#include "../biome/Biome.h"
#include "../block/LayerBlock.h"
#include "TerrainGenerator.h"

SandLayerStage::SandLayerStage(TerrainGenerator *generator)
{
    m_generator = generator;
    Random random((uint64_t) generator->GetSeed() ^ 0x5a4d1a7e);
    m_duneNoise = std::make_unique<PerlinNoise>(random, 3);
}

void SandLayerStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    Block *air            = Block::ByName("air");
    Block *sand           = Block::ByName("sand");
    LayerBlock *sandLayer = (LayerBlock *) Block::ByName("sand_layer");

    for (int x = 0; x < Chunk::s_sizeX; x++)
    {
        for (int z = 0; z < Chunk::s_sizeZ; z++)
        {
            if (!chunk.GetSurfaceBiome(x, z)->HasDesertPlants())
            {
                continue;
            }

            int y = TerrainGenerator::FindTopSolid(chunk, x, z);
            if (y < 0 || y >= Chunk::s_sizeY - 1 || Block::ById(chunk.GetBlockId(x, y, z)) != sand ||
                Block::ById(chunk.GetBlockId(x, y + 1, z)) != air)
            {
                continue;
            }

            // We stretch the noise along x and fold it around zero so dunes form long ridges with bare troughs
            double worldX = (double) (pos.x * Chunk::s_sizeX + x);
            double worldZ = (double) (pos.z * Chunk::s_sizeZ + z);
            double ridge  = 1.0 - std::abs(m_duneNoise->GetValue(worldX / 16.0, worldZ / 6.0) / 7.0);
            int layers    = (int) ((ridge - 0.55) * 10.0);
            if (layers <= 0)
            {
                continue;
            }

            chunk.SetBlock(x, y + 1, z, sandLayer);
            chunk.SetBlockData(x, y + 1, z, sandLayer->WithLayers(std::min(layers, 4)));
        }
    }
}
