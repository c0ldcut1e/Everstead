#include "DungeonStage.h"

#include "../../entity/EntityFactory.h"
#include "../../tag/CompoundTag.h"
#include "../block/Block.h"
#include "../blockentity/ChestBlockEntity.h"
#include "../loot/LootTables.h"
#include "../save/WorldSave.h"
#include "TerrainGenerator.h"

DungeonStage::DungeonStage(TerrainGenerator *generator)
{
    m_generator        = generator;
    m_air              = Block::ByName("air");
    m_cobblestone      = Block::ByName("cobblestone");
    m_mossyCobblestone = Block::ByName("mossy_cobblestone");
    m_chest            = Block::ByName("chest");
    m_spawner          = Block::ByName("mob_spawner");
}

void DungeonStage::Apply(Chunk &chunk, const ChunkPos &pos)
{
    const int attempts = 8;
    const int margin   = 4;

    Random random(m_generator->GetChunkSeed(pos, 0x510e527fade682d1ULL));
    for (int i = 0; i < attempts; i++)
    {
        int x = random.NextInt(margin, Chunk::s_sizeX - 1 - margin);
        int y = random.NextInt(0, TerrainGenerator::s_genDepth - 1);
        int z = random.NextInt(margin, Chunk::s_sizeZ - 1 - margin);
        Place(chunk, pos, random, x, y, z);
    }
}

// Rooms reach at most 4 blocks out from the spawner, so the margin in Apply keeps every room inside this chunk
void DungeonStage::Place(Chunk &chunk, const ChunkPos &pos, Random &random, int x, int y, int z)
{
    const int height = 3;

    int radiusX = random.NextInt(2, 3);
    int radiusZ = random.NextInt(2, 3);
    if (y < 2 || y + height + 1 >= Chunk::s_sizeY)
    {
        return;
    }

    int openings = 0;
    for (int xx = x - radiusX - 1; xx <= x + radiusX + 1; xx++)
    {
        for (int yy = y - 1; yy <= y + height + 1; yy++)
        {
            for (int zz = z - radiusZ - 1; zz <= z + radiusZ + 1; zz++)
            {
                bool solid = Block::ById(chunk.GetBlockId(xx, yy, zz))->IsSolid();
                if ((yy == y - 1 || yy == y + height + 1) && !solid)
                {
                    return;
                }

                bool wall = xx == x - radiusX - 1 || xx == x + radiusX + 1 || zz == z - radiusZ - 1 || zz == z + radiusZ + 1;
                if (wall && yy == y && Block::ById(chunk.GetBlockId(xx, yy, zz)) == m_air && Block::ById(chunk.GetBlockId(xx, yy + 1, zz)) == m_air)
                {
                    openings++;
                }
            }
        }
    }

    if (openings < 1 || openings > 5)
    {
        return;
    }

    for (int xx = x - radiusX - 1; xx <= x + radiusX + 1; xx++)
    {
        for (int yy = y + height; yy >= y - 1; yy--)
        {
            for (int zz = z - radiusZ - 1; zz <= z + radiusZ + 1; zz++)
            {
                bool interior = xx != x - radiusX - 1 && xx != x + radiusX + 1 && yy != y - 1 && yy != y + height + 1 && zz != z - radiusZ - 1 &&
                                zz != z + radiusZ + 1;
                if (interior || !Block::ById(chunk.GetBlockId(xx, yy - 1, zz))->IsSolid())
                {
                    chunk.SetBlock(xx, yy, zz, m_air);
                    chunk.SetBlockData(xx, yy, zz, 0);
                }
                else if (Block::ById(chunk.GetBlockId(xx, yy, zz))->IsSolid())
                {
                    chunk.SetBlock(xx, yy, zz, yy == y - 1 && random.NextInt(0, 3) != 0 ? m_mossyCobblestone : m_cobblestone);
                    chunk.SetBlockData(xx, yy, zz, 0);
                }
            }
        }
    }

    const int offsets[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
    for (int chest = 0; chest < 2; chest++)
    {
        for (int attempt = 0; attempt < 3; attempt++)
        {
            int chestX = x + random.NextInt(-radiusX, radiusX);
            int chestZ = z + random.NextInt(-radiusZ, radiusZ);
            if (Block::ById(chunk.GetBlockId(chestX, y, chestZ)) != m_air)
            {
                continue;
            }

            int solidSides = 0;
            int facing     = 0;
            for (int i = 0; i < 4; i++)
            {
                if (Block::ById(chunk.GetBlockId(chestX + offsets[i][0], y, chestZ + offsets[i][1]))->IsSolid())
                {
                    solidSides++;
                    facing = i;
                }
            }

            if (solidSides == 1)
            {
                PlaceChest(chunk, pos, random, chestX, y, chestZ, facing);
                break;
            }
        }
    }

    const eENTITY_TYPE mobs[4] = {eEntityType_Skeleton, eEntityType_Zombie, eEntityType_Zombie, eEntityType_Spider};
    chunk.SetBlock(x, y, z, m_spawner);
    chunk.SetBlockData(x, y, z, 0);
    std::shared_ptr<CompoundTag> tag(new CompoundTag());
    tag->PutString("id", "mob_spawner");
    tag->PutInt("x", pos.x * Chunk::s_sizeX + x);
    tag->PutInt("y", y);
    tag->PutInt("z", pos.z * Chunk::s_sizeZ + z);
    tag->PutString("EntityId", EntityFactory::GetResourceLocation(mobs[random.NextInt(0, 3)]).ToString());
    std::vector<std::shared_ptr<CompoundTag>> blockEntities = chunk.TakeBlockEntities();
    blockEntities.push_back(tag);
    chunk.SetBlockEntities(std::move(blockEntities));
}

void DungeonStage::PlaceChest(Chunk &chunk, const ChunkPos &pos, Random &random, int x, int y, int z, int facing)
{
    chunk.SetBlock(x, y, z, m_chest);
    chunk.SetBlockData(x, y, z, m_chest->WithPropertyValue(0, m_chest->FindProperty("facing"), facing));

    LootContext context = {&random, {}, nullptr, false, false};
    std::vector<ItemInstance> loot;
    LootTables::ForChest("simple_dungeon")->GetRandomItems(context, &loot);
    std::vector<ItemInstance> slots(ChestBlockEntity::s_rows * 9);
    for (size_t i = 0; i < loot.size(); i++)
    {
        slots[random.NextInt(0, (int) slots.size() - 1)] = loot[i];
    }

    std::unique_ptr<ListTag> items(new ListTag());
    for (size_t i = 0; i < slots.size(); i++)
    {
        if (slots[i].IsEmpty())
        {
            continue;
        }

        std::unique_ptr<CompoundTag> itemTag = WorldSave::CreateItemTag(slots[i]);
        itemTag->PutByte("Slot", (int8_t) i);
        items->Add(std::move(itemTag));
    }

    std::shared_ptr<CompoundTag> tag(new CompoundTag());
    tag->PutString("id", "chest");
    tag->PutInt("x", pos.x * Chunk::s_sizeX + x);
    tag->PutInt("y", y);
    tag->PutInt("z", pos.z * Chunk::s_sizeZ + z);
    tag->Put("Items", std::move(items));
    std::vector<std::shared_ptr<CompoundTag>> blockEntities = chunk.TakeBlockEntities();
    blockEntities.push_back(tag);
    chunk.SetBlockEntities(std::move(blockEntities));
}
