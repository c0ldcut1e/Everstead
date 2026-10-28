#include "TreeStage.h"

#include <climits>

#include "../block/Block.h"
#include "TerrainGenerator.h"

TreeStage::TreeStage(TerrainGenerator *generator)
{
    m_generator = generator;
}

void TreeStage::CollectTreeAttempts(const ChunkPos &owner, std::vector<TreeAttempt> *attempts) const
{
    int ownerX = owner.x * Chunk::s_sizeX;
    int ownerZ = owner.z * Chunk::s_sizeZ;

    Random random(m_generator->GetChunkSeed(owner, 0x5851f42d4c957f2dULL));
    // Each spot rolls against its own biome's density so forest edges don't follow chunk lines
    const int maxTrees = 24;

    float densityScale = m_generator->GetSettings().treeDensity;
    for (int i = 0; i < maxTrees; i++)
    {
        TreeAttempt attempt;
        attempt.owner = owner;
        attempt.x     = ownerX + random.NextInt(0, Chunk::s_sizeX - 1);
        attempt.z     = ownerZ + random.NextInt(0, Chunk::s_sizeZ - 1);
        attempt.seed  = random.NextUInt64();
        float roll    = random.NextFloat() * (float) maxTrees;
        Biome *biome  = m_generator->GetBiomeSource().GetBiome(attempt.x, attempt.z);
        if (roll < biome->GetTreeDensity() * densityScale && biome->IsDecorated())
        {
            attempts->push_back(attempt);
        }
    }
}

bool TreeStage::IsTreeCrowded(const TreeAttempt &attempt, const std::vector<TreeAttempt> &attempts) const
{
    const int minTreeSpacing = 4;

    for (size_t i = 0; i < attempts.size(); i++)
    {
        const TreeAttempt &other = attempts[i];
        int dx                   = other.x - attempt.x;
        int dz                   = other.z - attempt.z;
        if (other.seed > attempt.seed && dx * dx + dz * dz < minTreeSpacing * minTreeSpacing)
        {
            return true;
        }
    }

    return false;
}

void TreeStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int treeRadius  = 9;
    const int ownerRadius = 2;

    Block *grassBlock = Block::ByName("grass_block");
    Block *podzol     = Block::ByName("podzol");
    Block *coarseDirt = Block::ByName("coarse_dirt");

    std::vector<TreeAttempt> attempts;
    for (int dx = -ownerRadius; dx <= ownerRadius; dx++)
    {
        for (int dz = -ownerRadius; dz <= ownerRadius; dz++)
        {
            CollectTreeAttempts(ChunkPos(pos.x + dx, pos.z + dz), &attempts);
        }
    }

    int chunkX = pos.x * Chunk::s_sizeX;
    int chunkZ = pos.z * Chunk::s_sizeZ;
    ChunkPos lakeOwner(INT_MIN, INT_MIN);
    LakeFeature::Plan lake;
    for (size_t i = 0; i < attempts.size(); i++)
    {
        const TreeAttempt &attempt = attempts[i];
        int x                      = attempt.x - chunkX;
        int z                      = attempt.z - chunkZ;
        if (x + treeRadius < 0 || z + treeRadius < 0 || x - treeRadius >= Chunk::s_sizeX || z - treeRadius >= Chunk::s_sizeZ)
        {
            continue;
        }

        if (IsTreeCrowded(attempt, attempts))
        {
            continue;
        }

        if (!(lakeOwner == attempt.owner))
        {
            m_generator->GetLakePlan(attempt.owner, &lake);
            lakeOwner = attempt.owner;
        }

        int surface = m_generator->GetTerrainHeight(attempt.x, attempt.z);
        int lakeX   = attempt.x - attempt.owner.x * Chunk::s_sizeX;
        int lakeZ   = attempt.z - attempt.owner.z * Chunk::s_sizeZ;
        if (surface < TerrainGenerator::s_seaLevel - 1 ||
            (m_generator->GetLakeFeature().Covers(lake, lakeX, lakeZ) && lake.baseY + LakeFeature::s_height >= surface))
        {
            continue;
        }

        // Every chunk a tree overlaps has to make the same call, so we check the generated ground instead of this chunk's blocks
        Biome *biome  = m_generator->GetBiomeSource().GetBiome(attempt.x, attempt.z);
        Block *ground = m_generator->GetSurfaceBlock(biome, attempt.x, attempt.z, surface);
        if (ground != grassBlock && ground != podzol && ground != coarseDirt)
        {
            continue;
        }

        Random treeRandom(attempt.seed);
        Biome::eTreeType type = biome->GetTreeType();
        int altChance         = biome->GetAltTreeChance();
        if (altChance > 0 && treeRandom.NextInt(0, altChance - 1) == 0)
        {
            type = biome->GetAltTreeType();
        }

        switch (type)
        {
            case Biome::eTreeType_SPRUCE:
                m_spruceTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                break;
            case Biome::eTreeType_MEGA_SPRUCE:
                // Every chunk the tree reaches has to agree on placing it, so we only check the generated height of the trunk columns
                if (treeRandom.NextInt(0, 2) == 0 && m_generator->GetTerrainHeight(attempt.x + 1, attempt.z) == surface &&
                    m_generator->GetTerrainHeight(attempt.x, attempt.z + 1) == surface &&
                    m_generator->GetTerrainHeight(attempt.x + 1, attempt.z + 1) == surface)
                {
                    m_megaSpruceTreeFeature.PlaceGenerated(chunk, treeRandom, x, surface + 1, z, treeRandom.NextInt(0, 12) == 0);
                }
                else
                {
                    m_spruceTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                }
                break;
            case Biome::eTreeType_DARK_OAK:
                if (m_generator->GetTerrainHeight(attempt.x + 1, attempt.z) == surface &&
                    m_generator->GetTerrainHeight(attempt.x, attempt.z + 1) == surface &&
                    m_generator->GetTerrainHeight(attempt.x + 1, attempt.z + 1) == surface)
                {
                    m_darkOakTreeFeature.PlaceGenerated(chunk, treeRandom, x, surface + 1, z);
                }
                else
                {
                    m_treeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                }
                break;
            case Biome::eTreeType_BIRCH:
                m_birchTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                break;
            case Biome::eTreeType_ACACIA:
                m_acaciaTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                break;
            case Biome::eTreeType_CHERRY:
                m_cherryTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                break;
            case Biome::eTreeType_SWAMP:
                m_swampTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                break;
            case Biome::eTreeType_JUNGLE:
            case Biome::eTreeType_JUNGLE_EDGE:
                if (treeRandom.NextInt(0, 9) == 0)
                {
                    m_treeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                }
                else if (treeRandom.NextInt(0, 3) == 0)
                {
                    m_jungleTreeFeature.PlaceBush(chunk, treeRandom, x, surface + 1, z);
                }
                else if (type == Biome::eTreeType_JUNGLE && treeRandom.NextInt(0, 2) == 0)
                {
                    m_jungleTreeFeature.PlaceMega(chunk, treeRandom, x, surface + 1, z);
                }
                else
                {
                    m_jungleTreeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                }
                break;
            default:
                m_treeFeature.Place(chunk, treeRandom, x, surface + 1, z);
                break;
        }
    }
}
