#include "WorldSave.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <thread>
#include <unordered_map>
#include <utility>

#include "../../io/FileSystem.h"
#include "../../rendering/PngEncoder.h"
#include "../../threading/ThreadPool.h"

#include "../../core/Logger.h"
#include "../../core/ResourceLocation.h"
#include "../../entity/Entity.h"
#include "../../entity/EntityFactory.h"
#include "../../entity/Inventory.h"
#include "../../entity/Mob.h"
#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/Player.h"
#include "../../entity/projectile/Arrow.h"
#include "../../network/PlayerList.h"
#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../tag/NbtIo.h"
#include "../../tag/StringTag.h"
#include "../../utils/Random.h"
#include "../../utils/math/Mth.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../chunk/Chunk.h"
#include "../chunk/ChunkPos.h"
#include "../chunk/ChunkSection.h"
#include "../item/Item.h"
#include "../lod/LodGenerator.h"
#include "DataFixer.h"
#include "RegionFile.h"
#include "SaveService.h"

bool WorldSave::Save(Level *level, const std::string &worldName, const PlayerList *playerList, std::atomic<float> *progress)
{
    File worldDirectory = CreateWorldDirectory(worldName);

    std::shared_ptr<CompoundTag> levelTag = CreateLevelDataTag(level, worldName);
    WriteCompressedTag(worldDirectory.GetChild("level.dat"), levelTag.get());
    worldDirectory.GetChild("playerdata").CreateDirectories();

    if (playerList)
    {
        const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
        for (size_t i = 0; i < players.size(); i++)
        {
            WritePlayerDataFile(worldDirectory, players[i].get());
        }
    }
    else
    {
        const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
        for (size_t i = 0; i < entities.size(); i++)
        {
            if (IsPlayerEntity(entities[i].get()))
            {
                WritePlayerDataFile(worldDirectory, (Player *) entities[i].get());
            }
        }
    }

    SaveDimension(level, worldName, progress);
    return true;
}

void WorldSave::SaveDimension(Level *level, const std::string &worldName, std::atomic<float> *progress)
{
    File worldDirectory = GetDimensionDirectory(worldName.empty() ? "New World" : worldName, level->GetDimensionId());
    worldDirectory.GetChild("entities").RemoveAll();
    worldDirectory.GetChild("region").CreateDirectories();
    worldDirectory.GetChild("entities").CreateDirectories();

    // Chunks fill the first 90 percent of the progress bar and entities the rest
    std::vector<std::pair<ChunkPos, Chunk *>> chunks = level->GetLoadedChunks();
    std::vector<Chunk *> dirtyChunks;
    for (size_t i = 0; i < chunks.size(); i++)
    {
        Chunk *chunk = chunks[i].second;
        level->StoreBlockEntities(chunk, false);
        if (chunk->NeedsSave() || !chunk->GetBlockEntities().empty())
        {
            level->StoreBlockTicks(chunk, false);
            dirtyChunks.push_back(chunk);
        }
    }

    size_t threadCount = GetWorkerThreadBudget();
    std::vector<std::vector<uint8_t>> blobs(dirtyChunks.size());
    if (!dirtyChunks.empty())
    {
        ThreadPool pool(std::min(threadCount, dirtyChunks.size()));
        std::atomic<size_t> encodedChunks{0};
        for (size_t i = 0; i < dirtyChunks.size(); i++)
        {
            pool.DetachTask(
                    [&dirtyChunks, &blobs, &encodedChunks, &worldName, level, i, progress]()
                    {
                        blobs[i] = NbtIo::WriteCompressedBytes(CreateChunkTag(dirtyChunks[i]).get());
                        if (level->GetDimensionId() == eDimensionId_Overworld)
                        {
                            LodGenerator::GetInstance()->GetStore()->Ingest(worldName, dirtyChunks[i], true);
                        }

                        size_t done = encodedChunks.fetch_add(1) + 1;
                        if (progress)
                        {
                            progress->store(0.8f * (float) done / (float) dirtyChunks.size());
                        }
                    });
        }

        pool.Wait();
    }

    std::map<std::pair<int, int>, std::vector<std::pair<int, std::vector<uint8_t>>>> chunkRegions;
    for (size_t i = 0; i < dirtyChunks.size(); i++)
    {
        const ChunkPos &pos = dirtyChunks[i]->GetPos();
        chunkRegions[std::make_pair(Mth::FloorDiv(pos.x, s_regionSize), Mth::FloorDiv(pos.z, s_regionSize))].emplace_back(
                RegionFile::LocalIndex(pos.x, pos.z), std::move(blobs[i]));
        dirtyChunks[i]->SetSaved(true);
        dirtyChunks[i]->ClearDirty();
    }

    File regionDirectory = worldDirectory.GetChild("region");
    for (auto regionIt = chunkRegions.begin(); regionIt != chunkRegions.end(); regionIt++)
    {
        std::string name = "r." + std::to_string(regionIt->first.first) + "." + std::to_string(regionIt->first.second) + ".evr";
        RegionFile::WriteChunks(regionDirectory.GetChild(name).GetPath(), regionIt->second);
    }

    if (progress)
    {
        progress->store(0.9f);
    }

    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    std::map<std::pair<int, int>, std::vector<Entity *>> entityRegions;
    for (auto entityIt = entities.begin(); entityIt != entities.end(); entityIt++)
    {
        Entity *entity = entityIt->get();
        if (IsPlayerEntity(entity))
        {
            continue;
        }

        Mob *mob     = dynamic_cast<Mob *>(entity);
        Arrow *arrow = dynamic_cast<Arrow *>(entity);
        if (arrow                                     ? arrow->IsCollecting()
            : mob                                     ? mob->GetHealth() <= 0
            : entity->GetType() == ItemEntity::s_type ? ((ItemEntity *) entity)->IsCollecting()
                                                      : entity->GetType() == eEntityType_LightningBolt || entity->GetType() == eEntityType_LeashKnot)
        {
            continue;
        }

        glm::dvec3 position = entity->GetPosition();
        entityRegions[std::make_pair(GetEntityRegionCoordinate(position.x), GetEntityRegionCoordinate(position.z))].push_back(entity);
    }

    worldDirectory.GetChild("entities").RemoveAll();

    size_t totalRegions = entityRegions.size();
    std::atomic<size_t> writtenFiles{0};

    if (totalRegions > 0)
    {
        ThreadPool pool(threadCount < totalRegions ? threadCount : totalRegions);

        for (auto regionIt = entityRegions.begin(); regionIt != entityRegions.end(); regionIt++)
        {
            int regionX                         = regionIt->first.first;
            int regionZ                         = regionIt->first.second;
            const std::vector<Entity *> *region = &regionIt->second;
            pool.DetachTask(
                    [&worldDirectory, regionX, regionZ, region, &writtenFiles, totalRegions, progress]()
                    {
                        WriteEntityRegionFile(worldDirectory, regionX, regionZ, *region);
                        size_t done = writtenFiles.fetch_add(1) + 1;
                        if (progress)
                        {
                            progress->store(0.9f + 0.1f * (float) done / (float) totalRegions);
                        }
                    });
        }

        pool.Wait();
    }

    if (progress)
    {
        progress->store(1.0f);
    }

    Logger::LogInfo("Saved dimension to %s (%zu chunks)", worldDirectory.GetPath().c_str(), chunks.size());
}

bool WorldSave::Load(Level *level, const std::string &worldName, std::atomic<float> *progress)
{
    File worldDirectory                   = GetWorldDirectory(worldName);
    std::unique_ptr<CompoundTag> levelTag = ReadCompressedTag(worldDirectory.GetChild("level.dat"));
    if (!levelTag)
    {
        Logger::LogInfo("Failed to read world metadata for %s", worldName.c_str());
        return false;
    }

    DataFixer::Update(eDataType_Level, levelTag.get());

    level->SetSeed((uint32_t) levelTag->GetInt("Seed"));
    level->SetWorldTime(levelTag->GetLong("Time"));
    level->SetWeather(levelTag->GetBoolean("raining"), levelTag->GetInt("rainTime"), levelTag->GetBoolean("thundering"),
                      levelTag->GetInt("thunderTime"));
    if (levelTag->Contains("RandomState"))
    {
        level->GetRandom().SetState((uint64_t) levelTag->GetLong("RandomState"));
    }

    if (levelTag->Contains("BorderSize"))
    {
        WorldBorder &border = level->GetWorldBorder();
        border.SetCenter(levelTag->GetDouble("BorderCenterX"), levelTag->GetDouble("BorderCenterZ"));
        border.LerpSizeBetween(levelTag->GetDouble("BorderSize"), levelTag->GetDouble("BorderSizeLerpTarget"),
                               levelTag->GetLong("BorderSizeLerpTime") / 50);
        border.SetDamagePerBlock(levelTag->GetDouble("BorderDamagePerBlock"));
        border.SetDamageBuffer(levelTag->GetDouble("BorderSafeZone"));
        border.SetWarningBlocks((int) levelTag->GetDouble("BorderWarningBlocks"));
        border.SetWarningTime((int) levelTag->GetDouble("BorderWarningTime"));
    }

    if (levelTag->Contains("Difficulty"))
    {
        level->SetDifficulty(levelTag->GetInt("Difficulty"));
    }

    if (levelTag->Contains("GameType") && GameType::IsValid(levelTag->GetInt("GameType")))
    {
        level->SetDefaultGameType((eGAME_TYPE) levelTag->GetInt("GameType"));
    }

    if (levelTag->Contains("MaxPlayers"))
    {
        level->SetOnlineGame(levelTag->GetBoolean("OnlineGame"));
        level->SetAllowFriendsOfFriends(levelTag->GetBoolean("AllowFriendsOfFriends"));
        level->SetMaxPlayers(levelTag->GetInt("MaxPlayers"));
    }

    if (levelTag->Contains("SpawnX"))
    {
        level->SetWorldSpawn(BlockPos(levelTag->GetInt("SpawnX"), levelTag->GetInt("SpawnY"), levelTag->GetInt("SpawnZ")));
    }

    CompoundTag *gameRules = levelTag->GetCompound("GameRules");
    if (gameRules)
    {
        level->GetGameRules().Load(gameRules);
    }

    level->GetScoreboard().Load(levelTag.get());

    CompoundTag *worldGen = levelTag->GetCompound("WorldGen");
    if (worldGen)
    {
        WorldGenSettings settings;
        settings.Load(worldGen);
        level->SetGenSettings(settings);
    }

    LoadDimensionEntities(level, worldName);

    if (progress)
    {
        progress->store(1.0f);
    }

    Logger::LogInfo("Loaded world metadata from %s (chunks stream on demand)", worldDirectory.GetPath().c_str());
    return true;
}

void WorldSave::LoadDimensionEntities(Level *level, const std::string &worldName)
{
    File entityDirectory = GetDimensionDirectory(worldName, level->GetDimensionId()).GetChild("entities");
    if (!entityDirectory.Exists())
    {
        return;
    }

    std::vector<File> files = entityDirectory.ListFiles();
    for (size_t i = 0; i < files.size(); i++)
    {
        if (files[i].IsRegularFile() && files[i].GetExtension() == ".evr")
        {
            LoadEntityRegionFile(level, files[i]);
        }
    }
}

void WorldSave::SavePlayerData(const std::string &worldName, Player *player)
{
    File worldDirectory = CreateWorldDirectory(worldName);
    worldDirectory.GetChild("playerdata").CreateDirectories();
    WritePlayerDataFile(worldDirectory, player);
}

std::shared_ptr<CompoundTag> WorldSave::CreatePlayerDataTag(Player *player)
{
    std::unique_ptr<CompoundTag> tag = CreatePlayerTag(player);
    return std::shared_ptr<CompoundTag>(tag.release());
}

std::vector<BannedPlayer> WorldSave::ReadBans(const std::string &worldName)
{
    std::vector<BannedPlayer> bannedPlayers;
    std::unique_ptr<CompoundTag> tag = ReadCompressedTag(GetWorldDirectory(worldName).GetChild("bans.dat"));
    ListTag *bans                    = tag ? tag->GetList("Banned") : nullptr;
    if (!bans)
    {
        return bannedPlayers;
    }

    for (size_t i = 0; i < bans->Size(); i++)
    {
        CompoundTag *banTag       = (CompoundTag *) bans->Get(i);
        std::vector<int> uniqueId = banTag->GetIntArray("UID");
        if (uniqueId.size() == 5)
        {
            bannedPlayers.push_back(
                    {UniqueID((uint32_t) uniqueId[0], (uint32_t) uniqueId[1], (uint32_t) uniqueId[2], (uint32_t) uniqueId[3], (uint32_t) uniqueId[4]),
                     banTag->GetString("Name")});
        }
    }

    return bannedPlayers;
}

void WorldSave::WriteBans(const std::string &worldName, const std::vector<BannedPlayer> &bannedPlayers)
{
    std::unique_ptr<ListTag> bans(new ListTag());
    for (size_t i = 0; i < bannedPlayers.size(); i++)
    {
        const uint32_t *uniqueId = bannedPlayers[i].uniqueID.GetData();
        std::unique_ptr<CompoundTag> banTag(new CompoundTag());
        banTag->PutIntArray("UID", {(int) uniqueId[0], (int) uniqueId[1], (int) uniqueId[2], (int) uniqueId[3], (int) uniqueId[4]});
        banTag->PutString("Name", bannedPlayers[i].name);
        bans->Add(std::move(banTag));
    }

    CompoundTag tag;
    tag.Put("Banned", std::move(bans));
    WriteCompressedTag(CreateWorldDirectory(worldName).GetChild("bans.dat"), &tag);
}

std::string WorldSave::GetPlayerDataFileName(Player *player)
{
    return player->GetUniqueID().ToString() + ".dat";
}

void WorldSave::WritePlayerData(const std::string &worldName, const std::string &fileName, const CompoundTag *tag)
{
    File worldDirectory = CreateWorldDirectory(worldName);
    worldDirectory.GetChild("playerdata").CreateDirectories();
    WriteCompressedTag(worldDirectory.GetChild("playerdata").GetChild(fileName), tag);
}

bool WorldSave::LoadPlayerData(const std::string &worldName, Player *player, eDIMENSION_ID *outDimension, std::vector<ItemInstance> *outBackpackItems)
{
    File path                        = GetWorldDirectory(worldName).GetChild("playerdata").GetChild(player->GetUniqueID().ToString() + ".dat");
    std::unique_ptr<CompoundTag> tag = ReadCompressedTag(path);
    if (!tag)
    {
        return false;
    }

    DataFixer::Update(eDataType_Player, tag.get());

    player->Entity::Load(tag.get());
    uint8_t dimension = (uint8_t) tag->GetByte("Dimension");
    *outDimension     = dimension < eDimensionId_Count ? (eDIMENSION_ID) dimension : eDimensionId_Overworld;
    if (tag->Contains("GameType") && GameType::IsValid(tag->GetInt("GameType")))
    {
        player->SetGameType((eGAME_TYPE) tag->GetInt("GameType"));
    }

    player->SetFlying(tag->GetBoolean("Flying") && GameType::CanFly(player->GetGameType()));
    if (tag->Contains("Health"))
    {
        player->SetHealth(tag->GetInt("Health"));
    }

    player->SetFallDistance(tag->GetFloat("FallDistance"));
    player->SetExperience(tag->GetFloat("XpP"), tag->GetInt("XpLevel"), tag->GetInt("XpTotal"));

    if (tag->Contains("foodLevel"))
    {
        player->GetFoodData()->Read(tag.get());
    }

    if (tag->Contains("SpawnX"))
    {
        uint8_t spawnDimension = (uint8_t) tag->GetByte("SpawnDimension");
        player->SetRespawnPosition(BlockPos(tag->GetInt("SpawnX"), tag->GetInt("SpawnY"), tag->GetInt("SpawnZ")),
                                   spawnDimension < eDimensionId_Count ? (eDIMENSION_ID) spawnDimension : eDimensionId_Overworld,
                                   tag->GetBoolean("SpawnForced"));
    }

    if (tag->GetBoolean("Operator"))
    {
        player->SetSharedFlag(eEntityFlag_Operator, true);
    }

    Inventory *inventory = player->GetInventory();
    ListTag *items       = tag->GetList("Inventory");
    for (size_t i = 0; items && i < items->Size(); i++)
    {
        const CompoundTag *itemTag = (const CompoundTag *) items->Get(i);
        int slot                   = (uint8_t) itemTag->GetByte("Slot");
        if (slot < Inventory::s_slotCount)
        {
            inventory->SetSlot(slot, ReadItemTag(itemTag));
        }

        ReadBackpackItems(itemTag, outBackpackItems);
    }

    ListTag *attributes = tag->GetList("attributes");
    for (size_t i = 0; attributes && i < attributes->Size(); i++)
    {
        const CompoundTag *attributeTag = (const CompoundTag *) attributes->Get(i);
        const Attribute *attribute      = Attribute::ByName(attributeTag->GetString("id"));
        if (!attribute)
        {
            continue;
        }

        AttributeInstance *instance = player->GetAttributes()->Get(*attribute);
        instance->SetBaseValue(attributeTag->GetDouble("base"));

        std::vector<AttributeModifier> modifiers;
        ListTag *modifierTags = attributeTag->GetList("modifiers");
        for (size_t j = 0; modifierTags && j < modifierTags->Size(); j++)
        {
            const CompoundTag *modifierTag = (const CompoundTag *) modifierTags->Get(j);
            AttributeModifier modifier{modifierTag->GetString("id"), modifierTag->GetDouble("amount"), eAttributeOperation_AddValue};
            if (AttributeInstance::ParseOperation(modifierTag->GetString("operation"), &modifier.operation))
            {
                modifiers.push_back(modifier);
            }
        }

        instance->SetModifiers(modifiers);
    }

    inventory->SetSelectedSlot(std::clamp(tag->GetInt("SelectedItemSlot"), 0, Inventory::s_hotbarSize - 1));
    CompoundTag *carriedTag = tag->GetCompound("Carried");
    if (carriedTag)
    {
        ItemInstance carried = ReadItemTag(carriedTag);
        inventory->AddItem(carried);
        ReadBackpackItems(carriedTag, outBackpackItems);
    }

    return true;
}

std::unique_ptr<CompoundTag> WorldSave::CreateItemTag(const ItemInstance &item)
{
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    tag->PutString("id", item.GetItem()->GetResourceLocation().ToString());
    tag->PutByte("Count", (int8_t) item.GetCount());
    tag->PutShort("Damage", (int16_t) item.GetAuxValue());
    if (item.HasTag())
    {
        tag->Put("tag", item.GetTag()->Clone());
    }

    return tag;
}

// Old saves can hold removed backpack items, so we pull out what was stored in them for the caller to drop
void WorldSave::ReadBackpackItems(const CompoundTag *itemTag, std::vector<ItemInstance> *out)
{
    CompoundTag *tag = itemTag->GetCompound("tag");
    if (Item::ByName(itemTag->GetString("id")) || !tag)
    {
        return;
    }

    ListTag *items = tag->GetList("Items");
    for (size_t i = 0; items && i < items->Size(); i++)
    {
        ItemInstance item = ReadItemTag((const CompoundTag *) items->Get(i));
        if (!item.IsEmpty())
        {
            out->push_back(item);
        }
    }
}

ItemInstance WorldSave::ReadItemTag(const CompoundTag *tag)
{
    Item *item = Item::ByName(tag->GetString("id"));
    if (!item)
    {
        return ItemInstance();
    }

    ItemInstance instance(item, tag->Contains("Count") ? (uint8_t) tag->GetByte("Count") : 1, (uint16_t) tag->GetShort("Damage"));
    CompoundTag *itemTag = tag->GetCompound("tag");
    if (itemTag)
    {
        instance.SetTag((CompoundTag *) itemTag->Clone().release());
    }

    return instance;
}

bool WorldSave::ReadPlayerSpawnPosition(const std::string &worldName, const UniqueID &uniqueID, glm::dvec3 *outPosition)
{
    File path                        = GetWorldDirectory(worldName).GetChild("playerdata").GetChild(uniqueID.ToString() + ".dat");
    std::unique_ptr<CompoundTag> tag = ReadCompressedTag(path);
    if (!tag)
    {
        return false;
    }

    DataFixer::Update(eDataType_Player, tag.get());

    ListTag *position = tag->GetList("Pos");
    *outPosition      = glm::dvec3(position->GetNumber(0), position->GetNumber(1), position->GetNumber(2));
    return true;
}

bool WorldSave::ReadInfo(const std::string &worldName, WorldInfo *info)
{
    File worldDirectory         = GetWorldDirectory(worldName);
    info->iconPath              = worldDirectory.GetChild("icon.png").GetPath();
    info->difficulty            = 2;
    info->defaultGameType       = eGameType_Survival;
    info->onlineGame            = true;
    info->allowFriendsOfFriends = false;
    info->maxPlayers            = 8;

    std::unique_ptr<CompoundTag> levelTag = ReadCompressedTag(worldDirectory.GetChild("level.dat"));
    if (!levelTag)
    {
        return false;
    }

    DataFixer::Update(eDataType_Level, levelTag.get());

    info->seed  = (uint32_t) levelTag->GetInt("Seed");
    info->valid = true;
    if (levelTag->Contains("Difficulty"))
    {
        info->difficulty = levelTag->GetInt("Difficulty");
    }

    if (levelTag->Contains("GameType") && GameType::IsValid(levelTag->GetInt("GameType")))
    {
        info->defaultGameType = levelTag->GetInt("GameType");
    }

    if (levelTag->Contains("MaxPlayers"))
    {
        info->onlineGame            = levelTag->GetBoolean("OnlineGame");
        info->allowFriendsOfFriends = levelTag->GetBoolean("AllowFriendsOfFriends");
        info->maxPlayers            = levelTag->GetInt("MaxPlayers");
    }

    CompoundTag *gameRules = levelTag->GetCompound("GameRules");
    if (gameRules)
    {
        info->gameRules.Load(gameRules);
    }

    return true;
}

bool WorldSave::WriteSettings(const WorldInfo &info)
{
    File levelFile                        = GetWorldDirectory(info.name).GetChild("level.dat");
    std::unique_ptr<CompoundTag> levelTag = ReadCompressedTag(levelFile);
    if (!levelTag)
    {
        return false;
    }

    DataFixer::Update(eDataType_Level, levelTag.get());
    levelTag->PutInt("Difficulty", info.difficulty);
    levelTag->PutInt("GameType", info.defaultGameType);
    levelTag->PutBoolean("OnlineGame", info.onlineGame);
    levelTag->PutBoolean("AllowFriendsOfFriends", info.allowFriendsOfFriends);
    levelTag->PutInt("MaxPlayers", info.maxPlayers);
    std::unique_ptr<CompoundTag> gameRules(new CompoundTag());
    info.gameRules.Save(gameRules.get());
    levelTag->Put("GameRules", std::move(gameRules));
    WriteCompressedTag(levelFile, levelTag.get());
    return true;
}

std::vector<WorldInfo> WorldSave::ListWorlds()
{
    std::vector<WorldInfo> worlds;

    File savesDirectory = GetSavesDirectory();
    if (!savesDirectory.Exists())
    {
        return worlds;
    }

    std::vector<File> files = savesDirectory.ListFiles();
    for (auto fileIt = files.begin(); fileIt != files.end(); fileIt++)
    {
        if (!fileIt->IsDirectory())
        {
            continue;
        }

        WorldInfo info;
        info.name         = fileIt->GetName();
        info.iconPath     = fileIt->GetChild("icon.png").GetPath();
        info.seed         = 0;
        info.lastModified = fileIt->GetChild("level.dat").GetModifiedTime();
        info.valid        = false;
        ReadInfo(info.name, &info);
        worlds.push_back(info);
    }

    std::sort(worlds.begin(), worlds.end(),
              [](const WorldInfo &a, const WorldInfo &b)
              {
                  return a.lastModified > b.lastModified;
              });

    return worlds;
}

std::string WorldSave::GetAvailableWorldName(const std::string &worldName)
{
    std::string baseName = worldName.empty() ? "New World" : worldName;
    File savesDirectory  = GetSavesDirectory();
    File target          = savesDirectory.GetChild(baseName);
    int suffix           = 1;
    while (target.Exists())
    {
        target = savesDirectory.GetChild(baseName + " (" + std::to_string(suffix) + ")");
        suffix++;
    }

    return target.GetName();
}

bool WorldSave::RenameWorld(const std::string &worldName, const std::string &newWorldName)
{
    std::string targetName = newWorldName.empty() ? worldName : newWorldName;
    if (targetName.empty() || worldName == targetName)
    {
        return false;
    }

    File source = GetWorldDirectory(worldName);
    File target = GetWorldDirectory(targetName);
    if (!source.Exists() || target.Exists())
    {
        return false;
    }

    return source.RenameTo(target);
}

bool WorldSave::DeleteWorld(const std::string &worldName)
{
    File worldDirectory = GetWorldDirectory(worldName);
    if (!worldDirectory.Exists())
    {
        return false;
    }

    return worldDirectory.RemoveAll();
}

bool WorldSave::DuplicateWorld(const std::string &worldName)
{
    File source = GetWorldDirectory(worldName);
    if (!source.Exists())
    {
        return false;
    }

    std::string targetName = GetAvailableWorldName(worldName + " copy");
    File target            = GetWorldDirectory(targetName);
    return source.CopyRecursiveTo(target);
}

size_t WorldSave::GetWorkerThreadBudget()
{
    size_t cores = (size_t) std::thread::hardware_concurrency();
    if (cores <= 3)
    {
        return 1;
    }

    return cores - 2;
}

File WorldSave::s_savesDirectory = FileSystem::GetUserDirectory().GetChild("saves");

void WorldSave::SetSavesDirectory(const File &directory)
{
    s_savesDirectory = directory;
}

File WorldSave::GetSavesDirectory()
{
    return s_savesDirectory;
}

File WorldSave::GetWorldDirectory(const std::string &worldName)
{
    return GetSavesDirectory().GetChild(worldName);
}

File WorldSave::GetDimensionDirectory(const std::string &worldName, eDIMENSION_ID dimension)
{
    // The overworld keeps the root layout so worlds saved before dimensions existed still load
    if (dimension == eDimensionId_Overworld)
    {
        return GetWorldDirectory(worldName);
    }

    return GetWorldDirectory(worldName).GetChild("dimensions").GetChild(kDimensionNames[dimension]);
}

bool WorldSave::SaveIcon(const std::string &worldName, const std::vector<uint8_t> &rgb, int width, int height)
{
    if (width <= 0 || height <= 0 || rgb.size() < (size_t) width * (size_t) height * 3)
    {
        return false;
    }

    std::vector<uint8_t> png = PngEncoder::Encode(rgb, width, height);
    if (png.empty())
    {
        return false;
    }

    File worldDirectory = GetWorldDirectory(worldName);
    return worldDirectory.GetChild("icon.png").WriteAllBytes(png);
}

File WorldSave::CreateWorldDirectory(const std::string &worldName)
{
    File target = GetWorldDirectory(worldName.empty() ? "New World" : worldName);
    target.CreateDirectories();
    return target;
}

void WorldSave::SaveChunkToRegion(const std::string &worldName, eDIMENSION_ID dimension, const Chunk *chunk)
{
    std::unique_ptr<CompoundTag> tag = CreateChunkTag(chunk);
    std::vector<uint8_t> blob        = NbtIo::WriteCompressedBytes(tag.get());
    if (dimension == eDimensionId_Overworld)
    {
        LodGenerator::GetInstance()->GetStore()->Ingest(worldName, chunk, true);
    }

    const ChunkPos &pos  = chunk->GetPos();
    File regionDirectory = GetDimensionDirectory(worldName, dimension).GetChild("region");
    regionDirectory.CreateDirectories();

    std::string name = "r." + std::to_string(Mth::FloorDiv(pos.x, s_regionSize)) + "." + std::to_string(Mth::FloorDiv(pos.z, s_regionSize)) + ".evr";
    RegionFile::WriteChunk(regionDirectory.GetChild(name).GetPath(), RegionFile::LocalIndex(pos.x, pos.z), blob);
}

void WorldSave::EnqueueChunkSave(SaveService *saveService, const std::string &worldName, eDIMENSION_ID dimension, const std::shared_ptr<Chunk> &chunk)
{
    std::tuple<std::string, int, int, int> key(worldName, (int) dimension, chunk->GetPos().x, chunk->GetPos().z);
    {
        std::lock_guard<std::mutex> lock(s_queuedChunksMutex);
        s_queuedChunks[key] = chunk;
    }

    saveService->Enqueue(
            [worldName, dimension, chunk, key]()
            {
                SaveChunkToRegion(worldName, dimension, chunk.get());
                std::lock_guard<std::mutex> lock(s_queuedChunksMutex);
                auto queued = s_queuedChunks.find(key);
                if (queued != s_queuedChunks.end() && queued->second == chunk)
                {
                    s_queuedChunks.erase(queued);
                }
            });
}

std::unique_ptr<Chunk> WorldSave::LoadChunkFromRegion(const std::string &worldName, eDIMENSION_ID dimension, int chunkX, int chunkZ)
{
    // A chunk can be requested again before its save job runs, and the region file would still hold the older copy
    {
        std::lock_guard<std::mutex> lock(s_queuedChunksMutex);
        auto queued = s_queuedChunks.find(std::make_tuple(worldName, (int) dimension, chunkX, chunkZ));
        if (queued != s_queuedChunks.end())
        {
            std::unique_ptr<Chunk> chunk = std::make_unique<Chunk>(*queued->second);
            chunk->SetSaved(true);
            chunk->ClearDirty();
            return chunk;
        }
    }

    File regionDirectory = GetDimensionDirectory(worldName, dimension).GetChild("region");
    std::string name =
            "r." + std::to_string(Mth::FloorDiv(chunkX, s_regionSize)) + "." + std::to_string(Mth::FloorDiv(chunkZ, s_regionSize)) + ".evr";
    File regionFile = regionDirectory.GetChild(name);
    if (!regionFile.Exists())
    {
        return nullptr;
    }

    std::vector<uint8_t> blob;
    if (!RegionFile::ReadChunk(regionFile.GetPath(), RegionFile::LocalIndex(chunkX, chunkZ), &blob))
    {
        return nullptr;
    }

    std::unique_ptr<CompoundTag> tag = NbtIo::ReadCompressedBytes(blob);
    if (!tag)
    {
        return nullptr;
    }

    DataFixer::Update(eDataType_Chunk, tag.get());

    std::unique_ptr<Chunk> chunk = ReadChunkTag(tag.get());
    if (chunk)
    {
        chunk->SetSaved(true);
        chunk->ClearDirty();
    }

    return chunk;
}

std::shared_ptr<CompoundTag> WorldSave::CreateLevelDataTag(Level *level, const std::string &worldName)
{
    std::shared_ptr<CompoundTag> tag = std::make_shared<CompoundTag>();
    tag->SetName("Data");
    DataFixer::Stamp(tag.get());
    tag->PutString("LevelName", worldName);
    tag->PutInt("Seed", (int) level->GetSeed());
    tag->PutLong("Time", level->GetWorldProvider().GetWorldTime());
    tag->PutBoolean("raining", level->IsRaining());
    tag->PutInt("rainTime", level->GetRainTime());
    tag->PutBoolean("thundering", level->IsThundering());
    tag->PutInt("thunderTime", level->GetThunderTime());
    tag->PutLong("RandomState", (int64_t) level->GetRandom().GetState());
    WorldBorder &border = level->GetWorldBorder();
    tag->PutDouble("BorderCenterX", border.GetCenterX());
    tag->PutDouble("BorderCenterZ", border.GetCenterZ());
    tag->PutDouble("BorderSize", border.GetSize());
    tag->PutDouble("BorderSizeLerpTarget", border.GetLerpTarget());
    tag->PutLong("BorderSizeLerpTime", border.GetLerpRemainingTicks() * 50);
    tag->PutDouble("BorderDamagePerBlock", border.GetDamagePerBlock());
    tag->PutDouble("BorderSafeZone", border.GetDamageBuffer());
    tag->PutDouble("BorderWarningBlocks", border.GetWarningBlocks());
    tag->PutDouble("BorderWarningTime", border.GetWarningTime());
    tag->PutInt("Difficulty", level->GetDifficulty());
    tag->PutInt("GameType", (int) level->GetDefaultGameType());
    tag->PutBoolean("OnlineGame", level->IsOnlineGame());
    tag->PutBoolean("AllowFriendsOfFriends", level->AllowsFriendsOfFriends());
    tag->PutInt("MaxPlayers", level->GetMaxPlayers());
    if (level->HasWorldSpawn())
    {
        tag->PutInt("SpawnX", level->GetWorldSpawn().x);
        tag->PutInt("SpawnY", level->GetWorldSpawn().y);
        tag->PutInt("SpawnZ", level->GetWorldSpawn().z);
    }

    std::unique_ptr<CompoundTag> gameRules(new CompoundTag());
    level->GetGameRules().Save(gameRules.get());
    tag->Put("GameRules", std::move(gameRules));
    level->GetScoreboard().Save(tag.get());
    std::unique_ptr<CompoundTag> worldGen(new CompoundTag());
    level->GetGenSettings().Save(worldGen.get());
    tag->Put("WorldGen", std::move(worldGen));
    return tag;
}

void WorldSave::WriteLevelData(const std::string &worldName, const CompoundTag *tag)
{
    File worldDirectory = GetWorldDirectory(worldName);
    worldDirectory.CreateDirectories();
    WriteCompressedTag(worldDirectory.GetChild("level.dat"), tag);
}

void WorldSave::WriteEntityRegionFile(const File &worldDirectory, int regionX, int regionZ, const std::vector<Entity *> &entities)
{
    CompoundTag regionTag;
    regionTag.SetName("Entities");
    DataFixer::Stamp(&regionTag);
    regionTag.PutInt("RegionX", regionX);
    regionTag.PutInt("RegionZ", regionZ);

    std::unique_ptr<ListTag> entityList(new ListTag());
    for (auto entityIt = entities.begin(); entityIt != entities.end(); entityIt++)
    {
        std::unique_ptr<CompoundTag> entityTag(new CompoundTag());
        entityTag->PutString("id", EntityFactory::GetResourceLocation((*entityIt)->GetType()).ToString());
        (*entityIt)->Save(entityTag.get());
        entityList->Add(std::move(entityTag));
    }

    regionTag.Put("Entities", std::move(entityList));

    std::string name = "r." + std::to_string(regionX) + "." + std::to_string(regionZ) + ".evr";
    WriteCompressedTag(worldDirectory.GetChild("entities").GetChild(name), &regionTag);
}

bool WorldSave::LoadEntityRegionFile(Level *level, const File &path)
{
    std::unique_ptr<CompoundTag> regionTag = ReadCompressedTag(path);
    if (!regionTag)
    {
        return false;
    }

    DataFixer::Update(eDataType_EntityRegion, regionTag.get());

    ListTag *entities = regionTag->GetList("Entities");
    if (!entities)
    {
        return false;
    }

    for (size_t i = 0; i < entities->Size(); i++)
    {
        Tag *entityTag = entities->Get(i);
        if (!entityTag || entityTag->GetType() != Tag::eTagType_Compound)
        {
            continue;
        }

        std::unique_ptr<Entity> entity = ReadEntityTag(level, (CompoundTag *) entityTag);
        if (entity)
        {
            level->AddEntity(std::move(entity));
        }
    }

    return true;
}

void WorldSave::WritePlayerDataFile(const File &worldDirectory, Player *player)
{
    std::unique_ptr<CompoundTag> playerTag = CreatePlayerTag(player);
    std::string fileName                   = player->GetUniqueID().ToString() + ".dat";
    WriteCompressedTag(worldDirectory.GetChild("playerdata").GetChild(fileName), playerTag.get());
}

std::unique_ptr<CompoundTag> WorldSave::CreateChunkTag(const Chunk *chunk)
{
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    tag->SetName("");
    DataFixer::Stamp(tag.get());

    const ChunkPos &pos = chunk->GetPos();
    tag->PutInt("x", pos.x);
    tag->PutInt("z", pos.z);

    std::unordered_map<uint32_t, uint32_t> localIds;
    std::unique_ptr<ListTag> blockNames(new ListTag());
    std::unique_ptr<ListTag> sections(new ListTag());
    std::vector<uint32_t> states((size_t) ChunkSection::s_volume);
    std::vector<uint8_t> skyLight((size_t) ChunkSection::s_volume);
    std::vector<uint8_t> blockLight((size_t) ChunkSection::s_volume * 3);
    for (int section = 0; section < Chunk::s_sectionCount; section++)
    {
        const ChunkSection &chunkSection = chunk->GetSection(section);
        for (int z = 0; z < ChunkSection::s_size; z++)
        {
            for (int y = 0; y < ChunkSection::s_size; y++)
            {
                for (int x = 0; x < ChunkSection::s_size; x++)
                {
                    size_t index   = (size_t) ChunkSection::Index(x, y, z);
                    uint32_t state = chunkSection.GetState((int) index);
                    auto inserted  = localIds.emplace(ChunkSection::StateId(state), (uint32_t) blockNames->Size());
                    if (inserted.second)
                    {
                        blockNames->Add(
                                std::unique_ptr<Tag>(new StringTag(Block::ById(ChunkSection::StateId(state))->GetResourceLocation().ToString())));
                    }

                    states[index]   = ChunkSection::MakeState(inserted.first->second, ChunkSection::StateData(state));
                    skyLight[index] = chunkSection.GetSkyLight(x, y, z);
                    chunkSection.GetBlockLight(x, y, z, &blockLight[index * 3], &blockLight[index * 3 + 1], &blockLight[index * 3 + 2]);
                }
            }
        }

        sections->Add(EncodeSection(section, states.data(), skyLight.data(), blockLight.data()));
    }

    tag->Put("BlockNames", std::move(blockNames));
    tag->Put("Sections", std::move(sections));
    tag->PutByteArray("Biomes", chunk->EncodeBiomes());

    std::unique_ptr<ListTag> blockTicks(new ListTag());
    const std::vector<ChunkBlockTick> &chunkBlockTicks = chunk->GetBlockTicks();
    for (size_t i = 0; i < chunkBlockTicks.size(); i++)
    {
        const ChunkBlockTick &tick = chunkBlockTicks[i];
        std::unique_ptr<CompoundTag> tickTag(new CompoundTag());
        tickTag->PutString("i", tick.block->GetResourceLocation().ToString());
        tickTag->PutInt("x", pos.x * Chunk::s_sizeX + tick.x);
        tickTag->PutInt("y", tick.y);
        tickTag->PutInt("z", pos.z * Chunk::s_sizeZ + tick.z);
        tickTag->PutInt("t", tick.delay);
        tickTag->PutInt("p", tick.priority);
        blockTicks->Add(std::move(tickTag));
    }

    tag->Put("block_ticks", std::move(blockTicks));

    std::unique_ptr<ListTag> blockEntities(new ListTag());
    const std::vector<std::shared_ptr<CompoundTag>> &chunkBlockEntities = chunk->GetBlockEntities();
    for (size_t i = 0; i < chunkBlockEntities.size(); i++)
    {
        blockEntities->Add(chunkBlockEntities[i]->Clone());
    }

    tag->Put("block_entities", std::move(blockEntities));
    return tag;
}

std::unique_ptr<CompoundTag> WorldSave::EncodeSection(int sectionY, const uint32_t *states, const uint8_t *skyLight, const uint8_t *blockLight)
{
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    tag->PutByte("Y", (int8_t) sectionY);

    std::unordered_map<uint32_t, int> paletteIndices;
    std::vector<int> palette;
    std::vector<int> indices((size_t) ChunkSection::s_volume);
    for (int i = 0; i < ChunkSection::s_volume; i++)
    {
        auto inserted = paletteIndices.emplace(states[i], (int) palette.size());
        if (inserted.second)
        {
            palette.push_back((int) states[i]);
        }

        indices[(size_t) i] = inserted.first->second;
    }

    if (palette.size() > 1 || palette[0] != 0)
    {
        int bits = palette.size() <= 16 ? 4 : palette.size() <= 256 ? 8 : 16;
        std::vector<uint8_t> packed((size_t) ChunkSection::s_volume * (size_t) bits / 8);
        for (int i = 0; i < ChunkSection::s_volume; i++)
        {
            PackIndices(&packed, bits, i, indices[(size_t) i]);
        }

        tag->PutIntArray("Palette", palette);
        tag->PutByte("Bits", (int8_t) bits);
        tag->PutByteArray("Indices", packed);
    }

    bool uniformSky = true;
    for (int i = 1; i < ChunkSection::s_volume && uniformSky; i++)
    {
        uniformSky = skyLight[i] == skyLight[0];
    }

    if (uniformSky)
    {
        tag->PutByte("UniformSkyLight", (int8_t) skyLight[0]);
    }
    else
    {
        std::vector<uint8_t> sky((size_t) ChunkSection::s_volume / 2);
        for (int i = 0; i < ChunkSection::s_volume; i++)
        {
            PackIndices(&sky, 4, i, skyLight[i]);
        }

        tag->PutByteArray("SkyLight", sky);
    }

    bool hasBlockLight = false;
    for (int i = 0; i < ChunkSection::s_volume * 3 && !hasBlockLight; i++)
    {
        hasBlockLight = blockLight[i] != 0;
    }

    if (hasBlockLight)
    {
        std::vector<uint8_t> light((size_t) ChunkSection::s_volume * 3 / 2);
        for (int i = 0; i < ChunkSection::s_volume * 3; i++)
        {
            PackIndices(&light, 4, i, blockLight[i]);
        }

        tag->PutByteArray("BlockLight", light);
    }

    return tag;
}

void WorldSave::PackIndices(std::vector<uint8_t> *out, int bits, int index, int value)
{
    if (bits == 4)
    {
        uint8_t &packed = (*out)[(size_t) (index >> 1)];
        packed          = (index & 1) ? (uint8_t) ((packed & 0x0F) | ((value & 0x0F) << 4)) : (uint8_t) ((packed & 0xF0) | (value & 0x0F));
    }
    else if (bits == 8)
    {
        (*out)[(size_t) index] = (uint8_t) value;
    }
    else
    {
        (*out)[(size_t) index * 2]     = (uint8_t) (value & 0xFF);
        (*out)[(size_t) index * 2 + 1] = (uint8_t) (value >> 8);
    }
}

int WorldSave::UnpackIndex(const std::vector<uint8_t> &packed, int bits, int index)
{
    if (bits == 4)
    {
        uint8_t value = packed[(size_t) (index >> 1)];
        return (index & 1) ? value >> 4 : value & 0x0F;
    }

    if (bits == 8)
    {
        return packed[(size_t) index];
    }

    return packed[(size_t) index * 2] | (packed[(size_t) index * 2 + 1] << 8);
}

void WorldSave::DecodeSection(const CompoundTag *tag, Chunk *chunk, const std::vector<uint32_t> &blockIds)
{
    int sectionY = tag->GetByte("Y");
    if (sectionY < 0 || sectionY >= Chunk::s_sectionCount)
    {
        return;
    }

    ChunkSection &section = chunk->GetSection(sectionY);
    if (tag->Contains("Palette"))
    {
        std::vector<int> palette     = tag->GetIntArray("Palette");
        std::vector<uint8_t> indices = tag->GetByteArray("Indices");
        int bits                     = tag->GetByte("Bits");
        if ((bits == 4 || bits == 8 || bits == 16) && indices.size() == (size_t) ChunkSection::s_volume * (size_t) bits / 8)
        {
            for (int i = 0; i < ChunkSection::s_volume; i++)
            {
                int paletteIndex = UnpackIndex(indices, bits, i);
                if (paletteIndex < (int) palette.size())
                {
                    uint32_t state = (uint32_t) palette[(size_t) paletteIndex];
                    section.SetState(i, ChunkSection::MakeState(blockIds[ChunkSection::StateId(state)], ChunkSection::StateData(state)));
                }
            }
        }
    }

    std::vector<uint8_t> sky = tag->GetByteArray("SkyLight");
    if (sky.size() == (size_t) ChunkSection::s_volume / 2)
    {
        for (int i = 0; i < ChunkSection::s_volume; i++)
        {
            section.SetSkyLight(i & 15, (i >> 4) & 15, i >> 8, (uint8_t) UnpackIndex(sky, 4, i));
        }
    }
    else
    {
        section.FillSkyLight((uint8_t) tag->GetByte("UniformSkyLight"));
    }

    std::vector<uint8_t> light = tag->GetByteArray("BlockLight");
    if (light.size() == (size_t) ChunkSection::s_volume * 3 / 2)
    {
        for (int i = 0; i < ChunkSection::s_volume; i++)
        {
            section.SetBlockLight(i & 15, (i >> 4) & 15, i >> 8, (uint8_t) UnpackIndex(light, 4, i * 3), (uint8_t) UnpackIndex(light, 4, i * 3 + 1),
                                  (uint8_t) UnpackIndex(light, 4, i * 3 + 2));
        }
    }
}

std::unique_ptr<Chunk> WorldSave::ReadChunkTag(const CompoundTag *tag)
{
    ListTag *sections = tag->GetList("Sections");
    if (!sections)
    {
        return nullptr;
    }

    // Blocks missing from this build load as air
    ListTag *blockNames = tag->GetList("BlockNames");
    std::vector<uint32_t> blockIds(blockNames->Size());
    for (size_t i = 0; i < blockNames->Size(); i++)
    {
        Block *block = Block::ByName(((StringTag *) blockNames->Get(i))->GetValue());
        blockIds[i]  = block ? block->GetId() : 0;
    }

    std::unique_ptr<Chunk> chunk(new Chunk(ChunkPos(tag->GetInt("x"), tag->GetInt("z"))));
    for (size_t i = 0; i < sections->Size(); i++)
    {
        Tag *section = sections->Get(i);
        if (section->GetType() == Tag::eTagType_Compound)
        {
            DecodeSection((const CompoundTag *) section, chunk.get(), blockIds);
        }
    }

    if (!chunk->DecodeBiomes(tag->GetByteArray("Biomes")))
    {
        return nullptr;
    }

    ListTag *blockTicks = tag->GetList("block_ticks");
    if (blockTicks)
    {
        const ChunkPos &pos = chunk->GetPos();
        std::vector<ChunkBlockTick> chunkBlockTicks;
        for (size_t i = 0; i < blockTicks->Size(); i++)
        {
            Tag *entry = blockTicks->Get(i);
            if (entry->GetType() != Tag::eTagType_Compound)
            {
                continue;
            }

            const CompoundTag *tickTag = (const CompoundTag *) entry;
            Block *block               = Block::ByName(tickTag->GetString("i"));
            int x                      = tickTag->GetInt("x") - pos.x * Chunk::s_sizeX;
            int y                      = tickTag->GetInt("y");
            int z                      = tickTag->GetInt("z") - pos.z * Chunk::s_sizeZ;
            if (!block || x < 0 || x >= Chunk::s_sizeX || z < 0 || z >= Chunk::s_sizeZ || y < 0 || y >= Chunk::s_sizeY)
            {
                continue;
            }

            chunkBlockTicks.push_back(ChunkBlockTick{block, x, y, z, tickTag->GetInt("t"), tickTag->GetInt("p")});
        }

        chunk->SetBlockTicks(std::move(chunkBlockTicks));
    }

    ListTag *blockEntities = tag->GetList("block_entities");
    if (blockEntities)
    {
        std::vector<std::shared_ptr<CompoundTag>> chunkBlockEntities;
        for (size_t i = 0; i < blockEntities->Size(); i++)
        {
            Tag *entry = blockEntities->Get(i);
            if (entry->GetType() == Tag::eTagType_Compound)
            {
                chunkBlockEntities.push_back(std::shared_ptr<CompoundTag>((CompoundTag *) entry->Clone().release()));
            }
        }

        chunk->SetBlockEntities(std::move(chunkBlockEntities));
    }

    return chunk;
}

std::unique_ptr<Entity> WorldSave::ReadEntityTag(Level *level, const CompoundTag *tag)
{
    eENTITY_TYPE type;
    if (!EntityFactory::ByName(ResourceLocation(tag->GetString("id")), &type))
    {
        return nullptr;
    }

    std::unique_ptr<Entity> entity = EntityFactory::Create(type, level);
    Random random;
    entity->ReadSpawnData(EntityFactory::CreateSpawnData(type, &random));
    entity->Load(tag);
    return entity;
}

std::unique_ptr<CompoundTag> WorldSave::CreatePlayerTag(Player *player)
{
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    tag->SetName("Player");
    player->Entity::Save(tag.get());
    DataFixer::Stamp(tag.get());
    tag->PutBoolean("Flying", player->IsFlying());
    tag->PutInt("GameType", (int) player->GetGameType());
    tag->PutByte("Dimension", (int8_t) player->GetLevel()->GetDimensionId());
    tag->PutInt("Health", player->GetHealth());
    tag->PutFloat("FallDistance", player->GetFallDistance());
    tag->PutFloat("XpP", player->GetExperienceProgress());
    tag->PutInt("XpLevel", player->GetExperienceLevel());
    tag->PutInt("XpTotal", player->GetTotalExperience());
    player->GetFoodData()->Write(tag.get());
    tag->PutBoolean("Operator", player->GetSharedFlag(eEntityFlag_Operator));
    if (player->HasRespawnPosition())
    {
        tag->PutInt("SpawnX", player->GetRespawnPosition().x);
        tag->PutInt("SpawnY", player->GetRespawnPosition().y);
        tag->PutInt("SpawnZ", player->GetRespawnPosition().z);
        tag->PutBoolean("SpawnForced", player->IsRespawnForced());
        tag->PutByte("SpawnDimension", (int8_t) player->GetRespawnDimension());
    }

    Inventory *inventory = player->GetInventory();
    std::unique_ptr<ListTag> items(new ListTag());
    for (int slot = 0; slot < Inventory::s_slotCount; slot++)
    {
        ItemInstance *item = inventory->GetSlot(slot);
        if (!item->IsEmpty())
        {
            std::unique_ptr<CompoundTag> itemTag = CreateItemTag(*item);
            itemTag->PutByte("Slot", (int8_t) slot);
            items->Add(std::move(itemTag));
        }
    }

    tag->Put("Inventory", std::move(items));

    std::unique_ptr<ListTag> attributes(new ListTag());
    const std::vector<AttributeInstance> &instances = player->GetAttributes()->GetInstances();
    for (size_t i = 0; i < instances.size(); i++)
    {
        std::unique_ptr<CompoundTag> attributeTag(new CompoundTag());
        attributeTag->PutString("id", instances[i].GetAttribute()->GetResourceLocation().ToString());
        attributeTag->PutDouble("base", instances[i].GetBaseValue());

        std::unique_ptr<ListTag> modifiers(new ListTag());
        const std::vector<AttributeModifier> &attributeModifiers = instances[i].GetModifiers();
        for (size_t j = 0; j < attributeModifiers.size(); j++)
        {
            std::unique_ptr<CompoundTag> modifierTag(new CompoundTag());
            modifierTag->PutString("id", attributeModifiers[j].id);
            modifierTag->PutDouble("amount", attributeModifiers[j].amount);
            modifierTag->PutString("operation", AttributeInstance::GetOperationName(attributeModifiers[j].operation));
            modifiers->Add(std::move(modifierTag));
        }

        attributeTag->Put("modifiers", std::move(modifiers));
        attributes->Add(std::move(attributeTag));
    }

    tag->Put("attributes", std::move(attributes));
    tag->PutInt("SelectedItemSlot", inventory->GetSelectedSlot());
    if (!inventory->GetCarried().IsEmpty())
    {
        tag->Put("Carried", CreateItemTag(inventory->GetCarried()));
    }
    return tag;
}

bool WorldSave::IsPlayerEntity(Entity *entity)
{
    switch (entity->GetType())
    {
        case eEntityType_Player:
        case eEntityType_LocalPlayer:
        case eEntityType_MultiPlayerLocalPlayer:
        case eEntityType_ServerPlayer:
        case eEntityType_RemotePlayer: {
            return true;
        }

        default: {
            return false;
        }
    }
}

int WorldSave::GetEntityRegionCoordinate(double value)
{
    int chunk = Mth::FloorDiv((int) floor(value), Chunk::s_sizeX);
    return Mth::FloorDiv(chunk, s_regionSize);
}

void WorldSave::WriteCompressedTag(const File &path, const CompoundTag *tag)
{
    path.WriteAllBytes(NbtIo::WriteCompressedBytes(tag));
}

std::unique_ptr<CompoundTag> WorldSave::ReadCompressedTag(const File &path)
{
    return NbtIo::ReadCompressedBytes(path.ReadAllBytes());
}
