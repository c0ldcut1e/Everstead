#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

#include "../../io/File.h"
#include "../../network/PlayerList.h"
#include "../../utils/UniqueID.h"
#include "../DimensionId.h"
#include "../GameRules.h"
#include "../item/ItemInstance.h"

#include <glm/vec3.hpp>

class Level;
class Chunk;
class CompoundTag;
class Entity;
class Player;
class SaveService;

struct WorldInfo
{
    std::string name;
    std::string iconPath;
    uint32_t seed;
    int64_t lastModified;
    int difficulty;
    int defaultGameType;
    bool onlineGame;
    bool allowFriendsOfFriends;
    int maxPlayers;
    GameRules gameRules;
    bool valid;
};

class WorldSave
{
public:
    static void SetSavesDirectory(const File &directory);
    static File GetWorldDirectory(const std::string &worldName);
    static File GetDimensionDirectory(const std::string &worldName, eDIMENSION_ID dimension);
    static bool Save(Level *level, const std::string &worldName, const PlayerList *playerList, std::atomic<float> *progress);
    static void SaveDimension(Level *level, const std::string &worldName, std::atomic<float> *progress);
    static bool Load(Level *level, const std::string &worldName, std::atomic<float> *progress);
    static void LoadDimensionEntities(Level *level, const std::string &worldName);
    static void SavePlayerData(const std::string &worldName, Player *player);
    static std::shared_ptr<CompoundTag> CreatePlayerDataTag(Player *player);
    static std::string GetPlayerDataFileName(Player *player);
    static void WritePlayerData(const std::string &worldName, const std::string &fileName, const CompoundTag *tag);
    static bool LoadPlayerData(const std::string &worldName, Player *player, eDIMENSION_ID *outDimension,
                               std::vector<ItemInstance> *outBackpackItems);
    static std::vector<BannedPlayer> ReadBans(const std::string &worldName);
    static void WriteBans(const std::string &worldName, const std::vector<BannedPlayer> &bannedPlayers);
    static bool ReadPlayerSpawnPosition(const std::string &worldName, const UniqueID &uniqueID, glm::dvec3 *outPosition);
    static bool ReadInfo(const std::string &worldName, WorldInfo *info);
    static bool WriteSettings(const WorldInfo &info);
    static std::vector<WorldInfo> ListWorlds();
    static std::string GetAvailableWorldName(const std::string &worldName);
    static bool RenameWorld(const std::string &worldName, const std::string &newWorldName);
    static bool DeleteWorld(const std::string &worldName);
    static bool DuplicateWorld(const std::string &worldName);
    static bool SaveIcon(const std::string &worldName, const std::vector<uint8_t> &rgb, int width, int height);

    static void SaveChunkToRegion(const std::string &worldName, eDIMENSION_ID dimension, const Chunk *chunk);
    static void EnqueueChunkSave(SaveService *saveService, const std::string &worldName, eDIMENSION_ID dimension,
                                 const std::shared_ptr<Chunk> &chunk);
    static std::unique_ptr<Chunk> LoadChunkFromRegion(const std::string &worldName, eDIMENSION_ID dimension, int chunkX, int chunkZ);
    static std::unique_ptr<CompoundTag> EncodeSection(int sectionY, const uint32_t *states, const uint8_t *skyLight, const uint8_t *blockLight);
    static std::shared_ptr<CompoundTag> CreateLevelDataTag(Level *level, const std::string &worldName);
    static void WriteLevelData(const std::string &worldName, const CompoundTag *tag);
    static std::unique_ptr<CompoundTag> CreateItemTag(const ItemInstance &item);
    static ItemInstance ReadItemTag(const CompoundTag *tag);

private:
    static const int s_regionSize = 32;
    static File s_savesDirectory;
    static inline std::mutex s_queuedChunksMutex;
    static inline std::map<std::tuple<std::string, int, int, int>, std::shared_ptr<Chunk>> s_queuedChunks;

    static size_t GetWorkerThreadBudget();
    static void ReadBackpackItems(const CompoundTag *itemTag, std::vector<ItemInstance> *out);
    static File GetSavesDirectory();
    static File CreateWorldDirectory(const std::string &worldName);
    static void WriteEntityRegionFile(const File &worldDirectory, int regionX, int regionZ, const std::vector<Entity *> &entities);
    static bool LoadEntityRegionFile(Level *level, const File &path);
    static void WritePlayerDataFile(const File &worldDirectory, Player *player);
    static std::unique_ptr<CompoundTag> CreateChunkTag(const Chunk *chunk);
    static std::unique_ptr<Chunk> ReadChunkTag(const CompoundTag *tag);
    static void DecodeSection(const CompoundTag *tag, Chunk *chunk, const std::vector<uint32_t> &blockIds);
    static void PackIndices(std::vector<uint8_t> *out, int bits, int index, int value);
    static int UnpackIndex(const std::vector<uint8_t> &packed, int bits, int index);
    static std::unique_ptr<Entity> ReadEntityTag(Level *level, const CompoundTag *tag);
    static std::unique_ptr<CompoundTag> CreatePlayerTag(Player *player);
    static bool IsPlayerEntity(Entity *entity);
    static int GetEntityRegionCoordinate(double value);
    static void WriteCompressedTag(const File &path, const CompoundTag *tag);
    static std::unique_ptr<CompoundTag> ReadCompressedTag(const File &path);
};
