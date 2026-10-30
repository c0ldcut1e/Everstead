#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ClientConnection.h"

#include <algorithm>
#include <cmath>

#include "../../core/Everstead.h"
#include "../../core/GameInfo.h"
#include "../../core/Logger.h"
#include "../../entity/EntityFactory.h"
#include "../../entity/Mob.h"
#include "../../entity/item/Boat.h"
#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/LocalPlayer.h"
#include "../../entity/player/RemotePlayer.h"
#include "../../entity/projectile/Arrow.h"
#include "../../entity/projectile/ThrowableProjectile.h"
#include "../../rendering/GameRenderer.h"
#include "../../sound/SoundEngine.h"
#include "../../ui/component/UIComponent_Hotbar.h"
#include "../../ui/control/UIController.h"
#include "../../ui/scene/UIScene_SignEdit.h"
#include "../../utils/Direction.h"
#include "../../utils/StringUtils.h"
#include "../../utils/UniqueID.h"
#include "../../utils/math/Mth.h"
#include "../../world/Explosion.h"
#include "../../world/Level.h"
#include "../../world/block/Block.h"
#include "../../world/blockentity/ChestBlockEntity.h"
#include "../../world/blockentity/MobSpawnerBlockEntity.h"
#include "../../world/blockentity/SignBlockEntity.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/inventory/InventoryMenu.h"
#include "../../world/lod/LodGenerator.h"
#include "../../world/particle/ParticleEngine.h"
#include "../Connection.h"
#include "../packet/Packet.h"
#include "../packet/c2s/KeepAliveC2SPacket.h"
#include "../packet/c2s/LoginC2SPacket.h"
#include "../packet/c2s/PreLoginC2SPacket.h"
#include "../packet/s2c/AddEntityS2CPacket.h"
#include "../packet/s2c/AddPlayerS2CPacket.h"
#include "../packet/s2c/BlockUpdateS2CPacket.h"
#include "../packet/s2c/ChangeDimensionS2CPacket.h"
#include "../packet/s2c/ChestOpenS2CPacket.h"
#include "../packet/s2c/ChunkBlocksUpdateS2CPacket.h"
#include "../packet/s2c/InfoS2CPacket.h"
#include "../packet/s2c/LevelReadyS2CPacket.h"
#include "../packet/s2c/LoginS2CPacket.h"
#include "../packet/s2c/MoveEntityPosRotS2CPacket.h"
#include "../packet/s2c/MoveEntityPosS2CPacket.h"
#include "../packet/s2c/MoveEntityRotS2CPacket.h"
#include "../packet/s2c/MovePlayerS2CPacket.h"
#include "../packet/s2c/OpenSignEditorS2CPacket.h"
#include "../packet/s2c/PlayerSleepS2CPacket.h"
#include "../packet/s2c/RemoveEntitiesS2CPacket.h"
#include "../packet/s2c/RemovePlayerS2CPacket.h"
#include "../packet/s2c/RotateHeadS2CPacket.h"
#include "../packet/s2c/SetEntityDataS2CPacket.h"
#include "../packet/s2c/SetPlayerPositionS2CPacket.h"
#include "../packet/s2c/SetWeatherS2CPacket.h"
#include "../packet/s2c/SignDataS2CPacket.h"
#include "../packet/s2c/SoundS2CPacket.h"
#include "../packet/s2c/SpawnerDataS2CPacket.h"
#include "../packet/s2c/TeamsS2CPacket.h"
#include "../packet/s2c/TeleportEntityS2CPacket.h"
#include "world/chunk/ChunkPos.h"

ClientConnection::ClientConnection(Connection *connection)
{
    m_connection                    = connection;
    m_pid                           = 0;
    m_username                      = "";
    m_uniqueID                      = {};
    m_remotePlayers                 = {};
    m_loggedIn                      = false;
    m_receivedInfo                  = false;
    m_receivedPlayerPosition        = false;
    m_teleportId                    = 0;
    m_unackedChunks                 = 0;
    m_ackedRenderDistance           = 0;
    m_levelReady                    = false;
    m_changingDimension             = false;
    m_staleChunks                   = {};
    m_disconnected                  = false;
    m_disconnectReason              = eDisconnectReason_None;
    m_playerIndex                   = 0;
    m_initialRemotePlayersRemaining = 0;
    m_lastPacketTime                = std::chrono::steady_clock::now();
    m_ticksSinceKeepAlive           = 0;
    m_worldName                     = "";
    m_serverName                    = "";
    m_maxPlayers                    = 0;
    m_tps                           = 20.0f;
    m_mspt                          = 0.0f;
    m_statusPlayers                 = {};
    Logger::LogInfo("Client connection created connection=%s", m_connection->GetName().c_str());
}

ClientConnection::~ClientConnection()
{
    delete m_connection;
}

void ClientConnection::Tick()
{
    m_connection->Tick();

    DimensionTransition *transition = Everstead::GetInstance()->GetGameRenderer()->GetDimensionTransition();
    if (!m_heldPackets.empty() && transition->IsCovered())
    {
        for (size_t i = 0; i < m_heldPackets.size(); i++)
        {
            m_heldPackets[i]->Handle(this);
        }

        m_heldPackets.clear();
    }

    int handledPackets             = 0;
    std::shared_ptr<Packet> packet = m_connection->PollReceivedPacket(true);
    while (packet)
    {
        // We hold the dimension change and everything after it until the portal covers the screen, so the new
        // dimension never shows through the opening
        if (m_heldPackets.empty() && m_levelReady && packet->GetId() == ePacketId_ChangeDimensionS2C && !transition->IsCovered())
        {
            transition->SetStability(((ChangeDimensionS2CPacket *) packet.get())->GetStability());
            Everstead::GetInstance()->StartDimensionTransition();
            m_heldPackets.push_back(packet);
        }
        else if (!m_heldPackets.empty())
        {
            m_heldPackets.push_back(packet);
        }
        else
        {
            packet->Handle(this);
        }

        handledPackets++;
        packet = m_connection->PollReceivedPacket(true);
    }

    // Stale chunks from the last dimension are only safe to free once their mesh rebuild finishes
    MultiPlayerLevel *level = Everstead::GetInstance()->GetLevel();
    for (auto it = m_staleChunks.begin(); it != m_staleChunks.end();)
    {
        if (level->IsChunkMeshBusy(*it))
        {
            it++;
            continue;
        }

        level->ReleaseChunkMeshes(*it);
        level->RemoveBlockEntities(*it);
        level->RemoveChunk(*it);
        auto parked = m_parkedChunks.find(*it);
        if (parked != m_parkedChunks.end())
        {
            level->ReceiveChunk(*it, std::move(parked->second));
            m_parkedChunks.erase(parked);
        }

        it = m_staleChunks.erase(it);
    }

    if (m_changingDimension && m_receivedPlayerPosition)
    {
        Everstead *everstead = Everstead::GetInstance();
        glm::dvec3 position  = everstead->GetLocalPlayer()->GetPosition();
        ChunkPos playerChunk =
                ChunkPos(Mth::FloorDiv((int) floor(position.x), Chunk::s_sizeX), Mth::FloorDiv((int) floor(position.z), Chunk::s_sizeZ));
        if (everstead->GetLevel()->HasChunk(playerChunk) && !m_staleChunks.count(playerChunk))
        {
            m_changingDimension = false;
            everstead->FinishDimensionTransition();
        }
    }

    // Tears start the transition before the server answers, so we close it again if the server never sends us anywhere
    if (transition->IsTraveling() && !m_changingDimension && m_heldPackets.empty() && transition->GetElapsedSeconds() > 5.0)
    {
        Everstead::GetInstance()->FinishDimensionTransition();
    }

    int renderDistance = Everstead::GetInstance()->options.renderDistance;
    if (m_unackedChunks > 0 || (m_levelReady && renderDistance != m_ackedRenderDistance))
    {
        Send(std::shared_ptr<Packet>(new ChunkAckC2SPacket(m_unackedChunks, renderDistance)));
        m_unackedChunks       = 0;
        m_ackedRenderDistance = renderDistance;
    }

    // We use the wall clock so catch-up ticks after a hitch can't time out a healthy connection
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (handledPackets > 0)
    {
        m_lastPacketTime = now;
    }

    if (now - m_lastPacketTime > std::chrono::seconds(s_timeoutSeconds) && !m_disconnected)
    {
        m_disconnected     = true;
        m_disconnectReason = eDisconnectReason_Timeout;
        Logger::LogInfo("Client connection timed out connection=%s timeoutSeconds=%d", m_connection->GetName().c_str(), s_timeoutSeconds);
    }

    if (!m_disconnected && m_connection->IsClosed())
    {
        // A disconnect packet can still be queued behind the per tick limit, and its reason says more than a plain closed socket
        std::shared_ptr<Packet> packet = m_connection->PollReceivedPacket(true);
        while (packet && !m_disconnected)
        {
            packet->Handle(this);
            packet = m_connection->PollReceivedPacket(true);
        }

        if (!m_disconnected)
        {
            m_disconnected     = true;
            m_disconnectReason = m_connection->GetDisconnectReason();
        }
    }

    m_ticksSinceKeepAlive++;
    if (m_ticksSinceKeepAlive >= s_keepAliveIntervalTicks)
    {
        m_ticksSinceKeepAlive = 0;
        m_connection->Send(std::shared_ptr<Packet>(new KeepAliveC2SPacket()));
    }
}

void ClientConnection::ForceDisconnect(eDISCONNECT_REASON reason)
{
    if (m_disconnected)
    {
        return;
    }

    m_loggedIn         = false;
    m_disconnected     = true;
    m_disconnectReason = reason;
    Logger::LogInfo("Client connection forced disconnect connection=%s reason=%u", m_connection->GetName().c_str(), (uint32_t) reason);
}

void ClientConnection::ResetTimeout()
{
    m_lastPacketTime = std::chrono::steady_clock::now();
}

void ClientConnection::BeginLogin(Pid pid, const std::string &username, const UniqueID &uniqueID)
{
    m_pid      = pid;
    m_username = username;
    m_uniqueID = uniqueID;
    Logger::LogInfo("Client connection begin login connection=%s pid=%u username=%s uid=%s", m_connection->GetName().c_str(), m_pid,
                    m_username.c_str(), m_uniqueID.ToString().c_str());
    m_connection->Send(std::shared_ptr<Packet>(new PreLoginC2SPacket(m_pid, m_username, g_gameVersionMajor, g_gameVersionMinor, g_gameVersionPatch,
                                                                     ModLoader::GetInstance()->GetSharedMods())));
    m_connection->Send(std::shared_ptr<Packet>(new LoginC2SPacket(m_pid, m_username, m_uniqueID)));
}

void ClientConnection::Send(std::shared_ptr<Packet> packet)
{
    m_connection->Send(packet);
}

bool ClientConnection::IsServerPacketListener()
{
    return false;
}

void ClientConnection::HandleDisconnect(DisconnectS2CPacket *packet)
{
    // Older hosts keep reading the login packet after a version rejection and send a second disconnect we have to ignore
    if (m_disconnected)
    {
        return;
    }

    m_loggedIn          = false;
    m_disconnected      = true;
    m_disconnectReason  = packet->GetReason();
    m_disconnectMessage = packet->GetText();
    Logger::LogInfo("Client connection disconnected connection=%s reason=%u", m_connection->GetName().c_str(), (uint32_t) m_disconnectReason);
}

void ClientConnection::HandleLogin(LoginS2CPacket *packet)
{
    (void) packet;

    Everstead::GetInstance()->GetLocalPlayer()->SetName(StringUtils::ToUtf16(m_username));
    Everstead::GetInstance()->GetLocalPlayer()->SetUniqueID(m_uniqueID);
    m_loggedIn     = true;
    m_disconnected = false;
    // Joined worlds are not ours to write into, so their LOD samples only live in memory for this session
    if (!Everstead::GetInstance()->GetMultiplayerSession()->IsHostingSession())
    {
        LodGenerator::GetInstance()->GetStore()->Open("", true);
    }

    Logger::LogInfo("Client connection login accepted connection=%s pid=%u username=%s", m_connection->GetName().c_str(), m_pid, m_username.c_str());
}

void ClientConnection::HandleInfo(InfoS2CPacket *packet)
{
    m_playerIndex = packet->GetPlayerIndex();
    m_worldName   = packet->GetWorldName();
    m_serverName  = packet->GetServerName();
    m_maxPlayers  = packet->GetMaxPlayers();
    Everstead::GetInstance()->GetLocalPlayer()->SetId(packet->GetEntityId());
    m_initialRemotePlayersRemaining = packet->GetPlayerCount() > 0 ? packet->GetPlayerCount() - 1 : 0;
    m_receivedInfo                  = true;
    Logger::LogInfo("Client connection info accepted connection=%s playerIndex=%u maxPlayers=%u "
                    "playerCount=%u world=%s",
                    m_connection->GetName().c_str(), m_playerIndex, packet->GetMaxPlayers(), packet->GetPlayerCount(),
                    packet->GetWorldName().c_str());
}

void ClientConnection::HandleLevelReady(LevelReadyS2CPacket *packet)
{
    (void) packet;

    m_levelReady = true;
    Logger::LogInfo("Client connection level ready connection=%s", m_connection->GetName().c_str());
}

void ClientConnection::HandleChunkBlocksUpdate(ChunkBlocksUpdateS2CPacket *packet)
{
    std::unique_ptr<Chunk> chunk = packet->TakeChunk();
    const ChunkPos &pos          = packet->GetChunkPos();
    Everstead *everstead         = Everstead::GetInstance();
    MultiPlayerLevel *level      = everstead->GetLevel();
    m_unackedChunks++;
    if (packet->GetDimension() != level->GetDimensionId())
    {
        return;
    }

    if (everstead->options.lodEnabled && !everstead->GetMultiplayerSession()->IsHostingSession() && level->GetDimensionId() == eDimensionId_Overworld)
    {
        LodGenerator::GetInstance()->GetStore()->Ingest("", chunk.get(), true);
    }

    // The server stops sending changes for chunks it forgot, so a resent chunk replaces the copy we kept
    if (m_staleChunks.count(pos) || (level->HasChunk(pos) && level->IsChunkMeshBusy(pos)))
    {
        m_staleChunks.insert(pos);
        m_parkedChunks[pos] = std::move(chunk);
        return;
    }

    if (level->HasChunk(pos))
    {
        level->ReleaseChunkMeshes(pos);
        level->RemoveBlockEntities(pos);
        level->RemoveChunk(pos);
    }

    level->ReceiveChunk(pos, std::move(chunk));
}

void ClientConnection::HandleSetPlayerPosition(SetPlayerPositionS2CPacket *packet)
{
    LocalPlayer *player = Everstead::GetInstance()->GetLocalPlayer();
    player->SetPosition(glm::dvec3(packet->GetX(), packet->GetY(), packet->GetZ()));
    player->ResetRotation(packet->GetYaw(), packet->GetPitch());
    player->SetVelocity(packet->GetVelocity());
    player->StoreOld();
    m_receivedPlayerPosition = true;
    m_teleportId             = packet->GetTeleportId();
    Logger::LogInfo("Client connection set player position connection=%s pos=%f,%f,%f", m_connection->GetName().c_str(), packet->GetX(),
                    packet->GetY(), packet->GetZ());
}

void ClientConnection::HandleAddPlayer(AddPlayerS2CPacket *packet)
{
    if (packet->GetPlayerIndex() == m_playerIndex)
    {
        return;
    }

    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    bool addedPlayer     = false;
    if (!player)
    {
        player      = AddRemotePlayer(packet->GetPlayerIndex(), packet->GetUsername());
        addedPlayer = true;
    }
    else if (!packet->GetUsername().empty())
    {
        player->SetName(StringUtils::ToUtf16(packet->GetUsername()));
    }

    player->SetId(packet->GetEntityId());
    player->ApplyNetworkState(glm::dvec3(packet->GetX(), packet->GetY(), packet->GetZ()), packet->GetYaw(), packet->GetPitch());
    player->SetSharedFlags(packet->GetFlags());
    if (addedPlayer && m_initialRemotePlayersRemaining > 0)
    {
        m_initialRemotePlayersRemaining--;
    }

    Logger::LogInfo("Client connection add remote player connection=%s playerIndex=%u "
                    "username=%s pos=%f,%f,%f yaw=%f pitch=%f",
                    m_connection->GetName().c_str(), packet->GetPlayerIndex(), packet->GetUsername().c_str(), packet->GetX(), packet->GetY(),
                    packet->GetZ(), packet->GetYaw(), packet->GetPitch());
}

void ClientConnection::HandleMovePlayer(MovePlayerS2CPacket *packet)
{
    if (packet->GetPlayerIndex() == m_playerIndex)
    {
        return;
    }

    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    if (!player)
    {
        player = AddRemotePlayer(packet->GetPlayerIndex(), "");
    }

    player->ApplyNetworkState(glm::dvec3(packet->GetX(), packet->GetY(), packet->GetZ()), packet->GetYaw(), packet->GetPitch());
}

void ClientConnection::HandleMoveEntityPos(MoveEntityPosS2CPacket *packet)
{
    Entity *entity = FindMovedEntity(packet->GetEntityId());
    if (entity)
    {
        entity->ApplyMoveEntity(packet->GetXa(), packet->GetYa(), packet->GetZa(), true, 0, 0, false, packet->IsOnGround());
    }
}

void ClientConnection::HandleMoveEntityRot(MoveEntityRotS2CPacket *packet)
{
    Entity *entity = FindMovedEntity(packet->GetEntityId());
    if (entity)
    {
        entity->ApplyMoveEntity(0, 0, 0, false, (int8_t) packet->GetYaw(), (int8_t) packet->GetPitch(), true, false);
    }
}

void ClientConnection::HandleMoveEntityPosRot(MoveEntityPosRotS2CPacket *packet)
{
    Entity *entity = FindMovedEntity(packet->GetEntityId());
    if (entity)
    {
        entity->ApplyMoveEntity(packet->GetXa(), packet->GetYa(), packet->GetZa(), true, (int8_t) packet->GetYaw(), (int8_t) packet->GetPitch(), true,
                                packet->IsOnGround());
    }
}

void ClientConnection::HandleRotateHead(RotateHeadS2CPacket *packet)
{
    LivingEntity *entity = dynamic_cast<LivingEntity *>(FindMovedEntity(packet->GetEntityId()));
    if (entity)
    {
        int headYaw = (int8_t) packet->GetHeadYaw();
        entity->ApplyHeadYaw((float) headYaw * 360.0f / 256.0f);
    }
}

void ClientConnection::HandleSetEntityMotion(SetEntityMotionS2CPacket *packet)
{
    Entity *entity = FindMovedEntity(packet->GetEntityId());
    if (entity)
    {
        entity->SetVelocity(packet->GetVelocity());
    }
}

void ClientConnection::HandleTeleportEntity(TeleportEntityS2CPacket *packet)
{
    Entity *entity = FindMovedEntity(packet->GetEntityId());
    if (entity)
    {
        int yaw   = (int8_t) packet->GetYaw();
        int pitch = (int8_t) packet->GetPitch();
        entity->ApplyTeleportEntity(packet->GetX(), packet->GetY(), packet->GetZ(), yaw, pitch, packet->IsOnGround());
    }
}

void ClientConnection::HandleSound(SoundS2CPacket *packet)
{
    glm::vec3 position = packet->GetPosition();
    Level *level       = Everstead::GetInstance()->GetLevel();
    level->PlaySound(packet->GetSoundName(), glm::dvec3(position.x, position.y, position.z), packet->GetVolume(), packet->GetPitch());
}

void ClientConnection::HandleStopSound(StopSoundS2CPacket *packet)
{
    SoundEngine *soundEngine = Everstead::GetInstance()->GetSoundEngine();
    if (soundEngine)
    {
        soundEngine->StopSounds(packet->GetSource(), packet->GetSoundId());
    }
}

void ClientConnection::HandleParticle(ParticleS2CPacket *packet)
{
    ParticleSpawnParams params;
    params.type     = packet->GetType();
    params.position = packet->GetPosition();
    if (packet->GetCount() == 0)
    {
        params.velocity    = packet->GetDelta() * (double) packet->GetSpeed();
        params.hasVelocity = true;
    }
    else
    {
        params.spread    = packet->GetDelta();
        params.dustSpeed = packet->GetSpeed();
        params.amount    = packet->GetCount();
    }

    Everstead::GetInstance()->GetLevel()->GetParticleEngine()->Spawn(params);
}

void ClientConnection::HandleSetGameMode(SetGameModeS2CPacket *packet)
{
    Everstead *everstead = Everstead::GetInstance();
    eGAME_TYPE gameType  = packet->GetGameType();
    bool tabsChanged     = GameType::HasInfiniteItems(gameType) != GameType::HasInfiniteItems(everstead->GetLocalPlayer()->GetGameType());
    everstead->GetLocalPlayer()->SetGameType(gameType);
    everstead->GetLocalPlayer()->SetFlying(packet->IsFlying());

    UIScene *topScene = everstead->GetUiController()->GetTopScene();
    if (topScene && topScene->GetId() == eUIScene_Inventory && (tabsChanged || GameType::IsSpectator(gameType)))
    {
        everstead->CloseInventory();
    }
}

void ClientConnection::HandleSetHealth(SetHealthS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (m_levelReady)
    {
        localPlayer->HurtTo(packet->GetHealth());
    }
    else
    {
        localPlayer->SetHealth(packet->GetHealth());
    }

    localPlayer->GetFoodData()->SetFoodLevel(packet->GetFoodLevel());
    localPlayer->GetFoodData()->SetSaturationLevel(packet->GetSaturationLevel());
}

void ClientConnection::HandleSetExperience(SetExperienceS2CPacket *packet)
{
    Everstead::GetInstance()->GetLocalPlayer()->SetExperience(packet->GetProgress(), packet->GetLevel(), packet->GetTotal());
}

void ClientConnection::HandleRemovePlayer(RemovePlayerS2CPacket *packet)
{
    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    if (!player)
    {
        return;
    }

    Everstead::GetInstance()->GetLevel()->RemoveEntity(player);
    m_remotePlayers.erase(packet->GetPlayerIndex());
    Logger::LogInfo("Client connection removed remote player connection=%s playerIndex=%u", m_connection->GetName().c_str(),
                    packet->GetPlayerIndex());
}

void ClientConnection::HandleBlockUpdate(BlockUpdateS2CPacket *packet)
{
    Level *level             = Everstead::GetInstance()->GetLevel();
    Block *block             = Block::ById(packet->GetBlockId());
    const BlockPos &blockPos = packet->GetPos();

    Block *air           = Block::ByName("air");
    Block *previousBlock = Block::ById(level->GetBlockId(blockPos));
    if (previousBlock && previousBlock->IsSelectable() && previousBlock != Block::ByName("moving_piston") && (!block || block == air))
    {
        Everstead::GetInstance()->GetLocalPlayer()->SpawnDestroyParticlesAt(blockPos);
    }

    if (previousBlock == Block::ByName("moving_piston") && block && block != air)
    {
        Everstead::GetInstance()->GetLevelRenderer()->AddSettlingBlock(blockPos, block, packet->GetBlockData());
    }

    level->SetBlock(blockPos, block, nullptr, packet->GetBlockData());
    if (block == Block::ByName("fracture_core"))
    {
        Everstead::GetInstance()->GetLevelRenderer()->AddTear(blockPos);
    }
}

void ClientConnection::HandleChat(ChatS2CPacket *packet)
{
    Everstead::GetInstance()->GetGui()->AddMessage(StringUtils::ToUtf16(packet->GetMessage()), packet->IsAdvanced());
}

void ClientConnection::HandlePlayerDeath(PlayerDeathS2CPacket *packet)
{
    if (!packet->GetMessage().empty())
    {
        Everstead::GetInstance()->GetGui()->AddMessage(StringUtils::ToUtf16(packet->GetMessage()), true);
    }

    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    if (player)
    {
        player->SetHealth(0);
    }
}

void ClientConnection::HandleMultiBlockUpdate(MultiBlockUpdateS2CPacket *packet)
{
    Level *level                                                  = Everstead::GetInstance()->GetLevel();
    LevelRenderer *levelRenderer                                  = Everstead::GetInstance()->GetLevelRenderer();
    Block *fractureCore                                           = Block::ByName("fracture_core");
    const std::vector<MultiBlockUpdateS2CPacket::Change> &changes = packet->GetChanges();
    for (size_t i = 0; i < changes.size(); i++)
    {
        Block *block        = Block::ById(changes[i].blockId);
        const BlockPos &pos = changes[i].pos;
        auto parked         = m_parkedChunks.find(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)));
        if (parked != m_parkedChunks.end())
        {
            parked->second->SetBlock(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y, Mth::FloorMod(pos.z, Chunk::s_sizeZ), block);
            parked->second->SetBlockData(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y, Mth::FloorMod(pos.z, Chunk::s_sizeZ), changes[i].blockData);
            continue;
        }

        // Our own edits are already predicted, so server changes like crop growth and redstone go through the budgeted mesh queue
        level->SetBlockDeferredMesh(changes[i].pos, block, changes[i].blockData);
        if (block == fractureCore)
        {
            levelRenderer->AddTear(changes[i].pos);
        }
    }

    levelRenderer->FlushMeshScheduling();
}

void ClientConnection::HandleUpdateAttributes(UpdateAttributesS2CPacket *packet)
{
    AttributeMap *attributes                                     = Everstead::GetInstance()->GetLocalPlayer()->GetAttributes();
    const std::vector<UpdateAttributesS2CPacket::Entry> &entries = packet->GetEntries();
    for (size_t i = 0; i < entries.size(); i++)
    {
        const Attribute *attribute = Attribute::ByName(entries[i].name);
        if (attribute)
        {
            AttributeInstance *instance = attributes->Get(*attribute);
            instance->SetBaseValue(entries[i].baseValue);
            instance->SetModifiers(entries[i].modifiers);
        }
    }
}

void ClientConnection::HandleTabComplete(TabCompleteS2CPacket *packet)
{
    Everstead::GetInstance()->GetGui()->ReceiveCompletions(packet->GetCompletions());
}

void ClientConnection::HandleSwing(SwingS2CPacket *packet)
{
    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    if (player)
    {
        player->Swing();
    }
}

void ClientConnection::HandleBlockDestruction(BlockDestructionS2CPacket *packet)
{
    Everstead::GetInstance()->GetLevelRenderer()->SetBlockDestruction(packet->GetPlayerIndex(), packet->GetPos(), packet->GetStage());
}

void ClientConnection::HandleAnimateHurt(AnimateHurtS2CPacket *packet)
{
    // Armor can absorb a hit down to no health change, so the local player still shakes from this packet
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (packet->GetEntityId() == localPlayer->GetId())
    {
        localPlayer->AnimateServerHurt();
        return;
    }

    LivingEntity *entity = dynamic_cast<LivingEntity *>(Everstead::GetInstance()->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (entity)
    {
        entity->AnimateHurt();
    }
}

void ClientConnection::HandleAnimateCrit(AnimateCritS2CPacket *packet)
{
    LivingEntity *entity = dynamic_cast<LivingEntity *>(Everstead::GetInstance()->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (entity)
    {
        entity->AnimateCrit();
    }
}

void ClientConnection::HandleKnockback(KnockbackS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    double sourceX           = packet->GetSourceX();
    double sourceZ           = packet->GetSourceZ();
    double distance          = sqrt(sourceX * sourceX + sourceZ * sourceZ);
    double strength          = 1.0 - localPlayer->GetAttributeValue(Attribute::s_knockbackResistance);
    if (strength <= 0.0)
    {
        return;
    }

    glm::dvec3 velocity = localPlayer->GetVelocity() * 0.5;
    velocity.x -= sourceX / distance * 0.4 * strength;
    velocity.y = std::min(velocity.y + 0.4 * strength, 0.4);
    velocity.z -= sourceZ / distance * 0.4 * strength;
    localPlayer->SetVelocity(velocity + packet->GetPush() * strength);
}

void ClientConnection::HandleExplode(ExplodeS2CPacket *packet)
{
    Explosion explosion(Everstead::GetInstance()->GetLevel(), packet->GetCenter(), packet->GetRadius());
    if (packet->GetType() == eExplosionType_RiftCharge)
    {
        explosion.SpawnChargeParticles();
    }
    else if (packet->GetType() == eExplosionType_RiftBurst)
    {
        explosion.SpawnBurstParticles();
        Everstead::GetInstance()->GetLevelRenderer()->GetWorldEffects().Add(WorldEffects::eWorldEffectType_RiftShockwave, packet->GetCenter(),
                                                                            packet->GetRadius());
    }
    else if (packet->GetType() == eExplosionType_RiftImplode)
    {
        explosion.SpawnImplodeParticles();
        Everstead::GetInstance()->GetLevelRenderer()->GetWorldEffects().Add(WorldEffects::eWorldEffectType_RiftImplosion, packet->GetCenter(),
                                                                            packet->GetRadius() * 0.5f);
    }
    else if (packet->GetType() == eExplosionType_RiftDetonate)
    {
        explosion.SpawnBurstParticles();
        Everstead::GetInstance()->GetLevelRenderer()->GetWorldEffects().Add(WorldEffects::eWorldEffectType_RiftDetonation, packet->GetCenter(),
                                                                            packet->GetRadius());
    }
    else if (packet->GetType() >= eExplosionType_ExperimentLaunch)
    {
        explosion.SpawnExperimentParticles(packet->GetType());
        WorldEffects::eWORLD_EFFECT_TYPE effects[] = {WorldEffects::eWorldEffectType_ExperimentTesseract,
                                                      WorldEffects::eWorldEffectType_ExperimentBurst,
                                                      WorldEffects::eWorldEffectType_Count,
                                                      WorldEffects::eWorldEffectType_ExperimentFinale,
                                                      WorldEffects::eWorldEffectType_SingularityCharge,
                                                      WorldEffects::eWorldEffectType_SingularityBlast,
                                                      WorldEffects::eWorldEffectType_NovaCharge,
                                                      WorldEffects::eWorldEffectType_NovaBlast,
                                                      WorldEffects::eWorldEffectType_TempestCharge,
                                                      WorldEffects::eWorldEffectType_TempestBlast,
                                                      WorldEffects::eWorldEffectType_PrismCharge,
                                                      WorldEffects::eWorldEffectType_PrismBlast,
                                                      WorldEffects::eWorldEffectType_HellgateCharge,
                                                      WorldEffects::eWorldEffectType_HellgateBlast};
        WorldEffects::eWORLD_EFFECT_TYPE effect    = effects[packet->GetType() - eExplosionType_ExperimentLaunch];
        if (effect != WorldEffects::eWorldEffectType_Count)
        {
            Everstead::GetInstance()->GetLevelRenderer()->GetWorldEffects().Add(effect, packet->GetCenter(), packet->GetRadius());
        }
    }
    else
    {
        explosion.SpawnParticles(packet->GetBlocks());
    }

    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    localPlayer->SetVelocity(localPlayer->GetVelocity() + packet->GetKnockback());
}

void ClientConnection::HandleServerStatus(ServerStatusS2CPacket *packet)
{
    m_tps           = packet->GetTps();
    m_mspt          = packet->GetMspt();
    m_maxPlayers    = packet->GetMaxPlayers();
    m_statusPlayers = packet->GetPlayers();
}

void ClientConnection::HandleChangeDimension(ChangeDimensionS2CPacket *packet)
{
    Everstead *everstead    = Everstead::GetInstance();
    MultiPlayerLevel *level = everstead->GetLevel();
    level->GetDynamicLights().Clear();
    if (!m_levelReady)
    {
        level->SetDimensionId(packet->GetDimension());
        return;
    }

    everstead->StartDimensionTransition();

    std::unordered_set<ChunkPos, ChunkPosHash> positions;
    level->CollectLoadedChunkPositions(&positions);
    m_staleChunks.clear();
    m_parkedChunks.clear();
    for (const ChunkPos &pos : positions)
    {
        if (level->IsChunkMeshBusy(pos))
        {
            m_staleChunks.insert(pos);
        }
        else
        {
            level->ReleaseChunkMeshes(pos);
            level->RemoveBlockEntities(pos);
            level->RemoveChunk(pos);
        }
    }

    LocalPlayer *localPlayer                             = everstead->GetLocalPlayer();
    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    std::vector<Entity *> removed;
    for (size_t i = 0; i < entities.size(); i++)
    {
        if (entities[i].get() != localPlayer)
        {
            removed.push_back(entities[i].get());
        }
    }

    for (size_t i = 0; i < removed.size(); i++)
    {
        level->RemoveEntity(removed[i]);
    }

    m_remotePlayers.clear();
    level->SetDimensionId(packet->GetDimension());
    m_receivedPlayerPosition = false;
    m_changingDimension      = true;
    Logger::LogInfo("Client connection changed dimension connection=%s dimension=%s", m_connection->GetName().c_str(),
                    kDimensionNames[packet->GetDimension()]);
}

void ClientConnection::HandleSetTime(SetTimeS2CPacket *packet)
{
    Level *level = Everstead::GetInstance()->GetLevel();
    level->SetWorldTime(packet->GetTicks());
    level->GetGameRules().Set(eGameRule_DoDaylightCycle, packet->IsDaylightCycle() ? 1 : 0);
}

void ClientConnection::HandleTickingState(TickingStateS2CPacket *packet)
{
    Everstead::GetInstance()->SetTickingState(packet->GetTickRate(), packet->IsFrozen());
}

void ClientConnection::HandleTickingStep(TickingStepS2CPacket *packet)
{
    Everstead::GetInstance()->GetTickRateManager()->SetFrozenTicksToRun(packet->GetSteps());
}

void ClientConnection::HandleStickFightScores(StickFightScoresS2CPacket *packet)
{
    Everstead::GetInstance()->SetStickFightScores(packet->GetScores());
}

void ClientConnection::HandleSetWeather(SetWeatherS2CPacket *packet)
{
    Level *level = Everstead::GetInstance()->GetLevel();
    level->SetWeather(packet->IsRaining(), 0, packet->IsThundering(), 0);
    level->SetWeatherLevels(packet->GetRainLevel(), packet->GetRainLevel() > 0.0f ? packet->GetThunderLevel() / packet->GetRainLevel() : 0.0f);
}

void ClientConnection::HandleWorldBorder(WorldBorderS2CPacket *packet)
{
    packet->Apply(&Everstead::GetInstance()->GetLevel()->GetWorldBorder());
}

void ClientConnection::HandleTeams(TeamsS2CPacket *packet)
{
    Everstead::GetInstance()->GetLevel()->GetScoreboard().Load(packet->GetTag());
}

void ClientConnection::HandleWorldEffect(WorldEffectS2CPacket *packet)
{
    Everstead::GetInstance()->GetLevelRenderer()->GetWorldEffects().Add(packet->GetType(), packet->GetCenter(), packet->GetRadius());
}

void ClientConnection::HandleDynamicLight(DynamicLightS2CPacket *packet)
{
    DynamicLights &dynamicLights = Everstead::GetInstance()->GetLevel()->GetDynamicLights();
    if (packet->IsRemoved())
    {
        dynamicLights.RemoveLight(packet->GetLight().id);
    }
    else
    {
        dynamicLights.SetLight(packet->GetLight());
    }
}

void ClientConnection::HandleFogVolume(FogVolumeS2CPacket *packet)
{
    DynamicLights &dynamicLights = Everstead::GetInstance()->GetLevel()->GetDynamicLights();
    if (packet->IsRemoved())
    {
        dynamicLights.RemoveFogVolume(packet->GetFogVolume().id);
    }
    else
    {
        dynamicLights.SetFogVolume(packet->GetFogVolume());
    }
}

void ClientConnection::HandleAtmosphere(AtmosphereS2CPacket *packet)
{
    Everstead::GetInstance()->GetLevel()->GetDynamicLights().SetAtmosphere(packet->GetAirDensity(), packet->GetAnisotropy());
}

void ClientConnection::HandleDynamicLightMenu(DynamicLightMenuS2CPacket *packet)
{
    (void) packet;
    Everstead::GetInstance()->ToggleNimbusMenu();
}

void ClientConnection::HandleAddEntity(AddEntityS2CPacket *packet)
{
    Level *level = Everstead::GetInstance()->GetLevel();
    if (!packet->IsValid())
    {
        return;
    }

    Entity *existing = level->GetEntityById(packet->GetEntityId());
    if (existing)
    {
        if (existing->GetType() == packet->GetType())
        {
            existing->SetPosition(packet->GetPosition());
            existing->SetRotation(packet->GetYaw(), packet->GetPitch());
            existing->SetVelocity(packet->GetVelocity());
            existing->ApplyNetworkState(packet->GetPosition(), packet->GetYaw(), packet->GetPitch());
            existing->ReadSpawnData(packet->GetData());
        }

        return;
    }

    std::unique_ptr<Entity> entity = EntityFactory::Create(packet->GetType(), level);
    if (!entity)
    {
        Logger::LogWarn("Client connection received unknown entity type=%u id=%d", (unsigned) packet->GetType(), packet->GetEntityId());
        return;
    }

    entity->SetId(packet->GetEntityId());
    entity->SetPosition(packet->GetPosition());
    entity->SetRotation(packet->GetYaw(), packet->GetPitch());
    entity->SetVelocity(packet->GetVelocity());
    entity->StoreOld();
    entity->ApplyNetworkState(packet->GetPosition(), packet->GetYaw(), packet->GetPitch());
    entity->ReadSpawnData(packet->GetData());
    level->AddEntity(std::move(entity));
}

void ClientConnection::HandleTakeItemEntity(TakeItemEntityS2CPacket *packet)
{
    Level *level   = Everstead::GetInstance()->GetLevel();
    Entity *entity = level->GetEntityById(packet->GetEntityId());
    Arrow *arrow   = dynamic_cast<Arrow *>(entity);
    if (arrow)
    {
        arrow->StartCollect(packet->GetPlayerIndex());
        return;
    }

    ItemEntity *itemEntity = dynamic_cast<ItemEntity *>(entity);
    if (!itemEntity || itemEntity->GetItem().IsEmpty())
    {
        return;
    }

    // A copy flies to the player while the real entity is removed or shrunk by the server
    std::unique_ptr<ItemEntity> collected(new ItemEntity(level));
    collected->StartCollect(itemEntity, packet->GetPlayerIndex(), packet->GetCount());
    level->AddEntity(std::move(collected));
}

void ClientConnection::HandleRemoveEntities(RemoveEntitiesS2CPacket *packet)
{
    Level *level                      = Everstead::GetInstance()->GetLevel();
    const std::vector<int> &entityIds = packet->GetEntityIds();
    for (size_t i = 0; i < entityIds.size(); i++)
    {
        Entity *entity                  = entityIds[i] > 0 ? level->GetEntityById(entityIds[i]) : nullptr;
        ThrowableProjectile *projectile = dynamic_cast<ThrowableProjectile *>(entity);
        Arrow *arrow                    = dynamic_cast<Arrow *>(entity);
        if (projectile)
        {
            projectile->RemoveAfterImpact();
        }
        else if (entity && !(arrow && arrow->IsCollecting()))
        {
            entity->OnRemoved();
            level->RemoveEntity(entity);
        }
    }
}

void ClientConnection::HandleSetEntityData(SetEntityDataS2CPacket *packet)
{
    if (packet->GetEntityId() != 0)
    {
        ItemEntity *itemEntity = dynamic_cast<ItemEntity *>(Everstead::GetInstance()->GetLevel()->GetEntityById(packet->GetEntityId()));
        if (itemEntity)
        {
            itemEntity->SetItem(packet->GetItem());
            itemEntity->SetAge(packet->GetAge());
            itemEntity->SetPickupDelay(packet->GetPickupDelay());
        }

        return;
    }

    if (packet->GetPlayerIndex() == m_playerIndex)
    {
        LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
        uint8_t localMask        = eEntityFlag_Sprinting | eEntityFlag_Sneaking | eEntityFlag_UsingItem;
        uint8_t localFlags       = localPlayer->GetSharedFlags() & localMask;
        localPlayer->SetSharedFlags((packet->GetFlags() & ~localMask) | localFlags);
        return;
    }

    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    if (player)
    {
        player->SetSharedFlags(packet->GetFlags());
    }
}

void ClientConnection::HandlePlayerSleep(PlayerSleepS2CPacket *packet)
{
    Player *player = packet->GetPlayerIndex() == m_playerIndex ? (Player *) Everstead::GetInstance()->GetLocalPlayer()
                                                               : FindRemotePlayer(packet->GetPlayerIndex());
    if (!player)
    {
        return;
    }

    if (packet->IsSleeping())
    {
        player->StartSleeping(packet->GetPos());
    }
    else
    {
        player->StopSleeping();
    }
}

void ClientConnection::HandleSpawnerData(SpawnerDataS2CPacket *packet)
{
    MobSpawnerBlockEntity *spawner = dynamic_cast<MobSpawnerBlockEntity *>(Everstead::GetInstance()->GetLevel()->GetBlockEntity(packet->GetPos()));
    if (spawner)
    {
        spawner->SetEntityType(packet->GetEntityType());
    }
}

void ClientConnection::HandleSignData(SignDataS2CPacket *packet)
{
    SignBlockEntity *sign = dynamic_cast<SignBlockEntity *>(Everstead::GetInstance()->GetLevel()->GetBlockEntity(packet->GetPos()));
    if (sign)
    {
        sign->Load(packet->GetTag());
    }
}

void ClientConnection::HandleMovingPiston(MovingPistonS2CPacket *packet)
{
    Level *level = Everstead::GetInstance()->GetLevel();
    level->SetBlockAndDataNoNeighborUpdates(packet->GetPos(), Block::ByName("moving_piston"), 0);
    BlockEntity *blockEntity = level->GetBlockEntity(packet->GetPos());
    if (blockEntity)
    {
        blockEntity->Load(packet->GetTag());
    }
}

void ClientConnection::HandleOpenSignEditor(OpenSignEditorS2CPacket *packet)
{
    Everstead *everstead = Everstead::GetInstance();
    everstead->OpenInventory(eUIScene_SignEdit);
    ((UIScene_SignEdit *) everstead->GetUiController()->GetTopScene())->SetSign(packet->GetPos(), packet->IsFront());
}

void ClientConnection::HandleSetMobData(SetMobDataS2CPacket *packet)
{
    Mob *mob = dynamic_cast<Mob *>(Everstead::GetInstance()->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (mob)
    {
        mob->ApplyMobData(packet->GetData());
    }
}

void ClientConnection::HandleEntityEvent(EntityEventS2CPacket *packet)
{
    Entity *entity = Everstead::GetInstance()->GetLevel()->GetEntityById(packet->GetEntityId());
    if (!entity)
    {
        return;
    }

    if (packet->GetEvent() == Entity::kAfterimageEvent)
    {
        Everstead::GetInstance()->GetLevelRenderer()->AddAfterimage(entity);
    }
    else
    {
        entity->HandleEntityEvent(packet->GetEvent());
    }
}

void ClientConnection::HandleSetPassengers(SetPassengersS2CPacket *packet)
{
    Level *level    = Everstead::GetInstance()->GetLevel();
    Entity *vehicle = level->GetEntityById(packet->GetVehicleId());
    if (!vehicle)
    {
        return;
    }

    Entity *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    bool wasController  = !vehicle->GetPassengers().empty() && vehicle->GetPassengers()[0] == localPlayer;
    while (!vehicle->GetPassengers().empty())
    {
        vehicle->GetPassengers()[0]->StopRiding();
    }

    const std::vector<int> &passengerIds = packet->GetPassengerIds();
    for (size_t i = 0; i < passengerIds.size(); i++)
    {
        Entity *passenger = level->GetEntityById(passengerIds[i]);
        if (passenger)
        {
            passenger->StartRiding(vehicle);
        }
    }

    if (wasController && (vehicle->GetPassengers().empty() || vehicle->GetPassengers()[0] != localPlayer))
    {
        vehicle->ApplyNetworkState(vehicle->GetPosition(), vehicle->GetYaw(), vehicle->GetPitch());
    }
}

void ClientConnection::HandleSetBoatData(SetBoatDataS2CPacket *packet)
{
    Boat *boat = dynamic_cast<Boat *>(Everstead::GetInstance()->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (boat)
    {
        boat->SetPaddleState(packet->GetLeftPaddle(), packet->GetRightPaddle());
    }
}

void ClientConnection::HandleContainerOpen(ContainerOpenS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (localPlayer && packet->GetMenuId() == InventoryMenu::s_craftingMenuId && !localPlayer->GetOpenContainer())
    {
        localPlayer->OpenContainer(localPlayer->GetCraftingContainer());
    }
    else if (localPlayer && packet->GetMenuId() == InventoryMenu::s_furnaceMenuId && !localPlayer->GetOpenContainer())
    {
        localPlayer->OpenContainer(localPlayer->GetFurnaceContainer());
    }
    else if (localPlayer && packet->GetMenuId() == InventoryMenu::s_resonantSocketMenuId && !localPlayer->GetOpenContainer())
    {
        localPlayer->OpenContainer(localPlayer->GetResonantSocketContainer());
    }
    else if (localPlayer && packet->GetMenuId() == InventoryMenu::s_resonatorMenuId && !localPlayer->GetOpenContainer())
    {
        localPlayer->OpenContainer(localPlayer->GetResonatorContainer());
    }
    else if (localPlayer && packet->GetMenuId() == InventoryMenu::s_anvilMenuId && !localPlayer->GetOpenContainer())
    {
        localPlayer->OpenContainer(localPlayer->GetAnvilContainer());
    }
    else if (localPlayer &&
             (packet->GetMenuId() == InventoryMenu::s_chestMenuId || packet->GetMenuId() == InventoryMenu::s_hopperMenuId ||
              packet->GetMenuId() == InventoryMenu::s_dispenserMenuId || packet->GetMenuId() == InventoryMenu::s_dropperMenuId) &&
             !localPlayer->GetOpenContainer())
    {
        localPlayer->OpenChest(packet->GetMenuId(), packet->GetRows());
    }
}

void ClientConnection::HandleChestOpen(ChestOpenS2CPacket *packet)
{
    ChestBlockEntity *chest = dynamic_cast<ChestBlockEntity *>(Everstead::GetInstance()->GetLevel()->GetBlockEntity(packet->GetPos()));
    if (chest)
    {
        chest->SetOpenCount(packet->GetOpenCount());
    }
}

void ClientConnection::HandleContainerClose(ContainerCloseS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (localPlayer &&
        ((packet->GetMenuId() == InventoryMenu::s_craftingMenuId && localPlayer->GetOpenContainer() == localPlayer->GetCraftingContainer()) ||
         (packet->GetMenuId() == InventoryMenu::s_furnaceMenuId && localPlayer->GetOpenContainer() == localPlayer->GetFurnaceContainer()) ||
         (packet->GetMenuId() == InventoryMenu::s_resonantSocketMenuId &&
          localPlayer->GetOpenContainer() == localPlayer->GetResonantSocketContainer()) ||
         (packet->GetMenuId() == InventoryMenu::s_resonatorMenuId && localPlayer->GetOpenContainer() == localPlayer->GetResonatorContainer()) ||
         (packet->GetMenuId() == InventoryMenu::s_anvilMenuId && localPlayer->GetOpenContainer() == localPlayer->GetAnvilContainer()) ||
         (packet->GetMenuId() == localPlayer->GetChestMenuId() && localPlayer->GetOpenContainer() &&
          localPlayer->GetOpenContainer() != localPlayer->GetCraftingContainer() &&
          localPlayer->GetOpenContainer() != localPlayer->GetFurnaceContainer() &&
          localPlayer->GetOpenContainer() != localPlayer->GetResonantSocketContainer() &&
          localPlayer->GetOpenContainer() != localPlayer->GetResonatorContainer() &&
          localPlayer->GetOpenContainer() != localPlayer->GetAnvilContainer())))
    {
        localPlayer->CloseContainer();
    }
}

void ClientConnection::HandleContainerSetContent(ContainerSetContentS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (!localPlayer)
    {
        return;
    }

    Inventory *inventory                   = localPlayer->GetInventory();
    const std::vector<ItemInstance> &items = packet->GetItems();
    inventory->SetCarried(packet->GetCarried());

    if (packet->GetMenuId() == InventoryMenu::s_chestMenuId || packet->GetMenuId() == InventoryMenu::s_craftingMenuId ||
        packet->GetMenuId() == InventoryMenu::s_furnaceMenuId || packet->GetMenuId() == InventoryMenu::s_resonantSocketMenuId ||
        packet->GetMenuId() == InventoryMenu::s_resonatorMenuId || packet->GetMenuId() == InventoryMenu::s_hopperMenuId ||
        packet->GetMenuId() == InventoryMenu::s_dispenserMenuId || packet->GetMenuId() == InventoryMenu::s_dropperMenuId ||
        packet->GetMenuId() == InventoryMenu::s_anvilMenuId)
    {
        AbstractContainer *container = localPlayer->GetOpenContainer();
        if (!container)
        {
            return;
        }

        for (int slot = 0; slot < container->GetSize(); slot++)
        {
            container->SetItem(slot, (size_t) slot < items.size() ? items[(size_t) slot] : ItemInstance());
        }

        return;
    }

    bool inventoryOpen = Everstead::GetInstance()->IsInventoryOpen();
    for (int slot = 0; slot < Inventory::s_slotCount; slot++)
    {
        ItemInstance item = (size_t) slot < items.size() ? items[(size_t) slot] : ItemInstance();
        ItemInstance *old = inventory->GetSlot(slot);
        if (!inventoryOpen && slot < Inventory::s_hotbarSize && !item.IsEmpty() &&
            (old->IsEmpty() || (old->GetItem() == item.GetItem() && item.GetCount() > old->GetCount())))
        {
            inventory->SetPopTime(slot, UIComponent_Hotbar::s_popDuration);
        }

        inventory->SetSlot(slot, item);
    }

    CraftingContainer *crafting = localPlayer->GetInventoryCraftingContainer();
    for (int slot = 0; slot < crafting->GetSize(); slot++)
    {
        size_t index = (size_t) (Inventory::s_slotCount + slot);
        crafting->SetItem(slot, index < items.size() ? items[index] : ItemInstance());
    }

    if (packet->GetSelectedSlot() < Inventory::s_hotbarSize)
    {
        inventory->SetSelectedSlot(packet->GetSelectedSlot());
    }
}

void ClientConnection::HandleContainerSetData(ContainerSetDataS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (!localPlayer)
    {
        return;
    }

    if (packet->GetMenuId() == InventoryMenu::s_resonatorMenuId)
    {
        localPlayer->GetResonatorContainer()->SetProgress(packet->GetValue());
        return;
    }

    if (packet->GetMenuId() == InventoryMenu::s_anvilMenuId)
    {
        localPlayer->GetAnvilContainer()->SetCost(packet->GetValue());
        return;
    }

    if (packet->GetMenuId() != InventoryMenu::s_furnaceMenuId)
    {
        return;
    }

    FurnaceContainer *furnace = localPlayer->GetFurnaceContainer();
    if (packet->GetDataId() == 0)
    {
        furnace->SetCookTime(packet->GetValue());
    }
    else if (packet->GetDataId() == 1)
    {
        furnace->SetLitTime(packet->GetValue());
    }
    else if (packet->GetDataId() == 2)
    {
        furnace->SetLitDuration(packet->GetValue());
    }
}

void ClientConnection::HandleContainerSetSlot(ContainerSetSlotS2CPacket *packet)
{
    LocalPlayer *localPlayer = Everstead::GetInstance()->GetLocalPlayer();
    if (!localPlayer || packet->GetSlot() >= Inventory::s_slotCount)
    {
        return;
    }

    localPlayer->GetInventory()->SetSlot(packet->GetSlot(), packet->GetItem());
}

void ClientConnection::HandleSetEquipment(SetEquipmentS2CPacket *packet)
{
    if (packet->GetPlayerIndex() == m_playerIndex)
    {
        return;
    }

    RemotePlayer *player = FindRemotePlayer(packet->GetPlayerIndex());
    if (!player)
    {
        return;
    }

    if (packet->GetSlot() > Inventory::s_armorSize)
    {
        return;
    }

    Inventory *inventory = player->GetInventory();
    if (packet->GetSlot() > 0)
    {
        *inventory->GetArmor(packet->GetSlot() - 1) = packet->GetItem();
        return;
    }

    inventory->SetSelectedSlot(0);
    inventory->SetItem(0, packet->GetItem());
}

Connection *ClientConnection::GetConnection() const
{
    return m_connection;
}

bool ClientConnection::IsLoggedIn() const
{
    return m_loggedIn;
}

bool ClientConnection::CanEnterWorld() const
{
    return m_loggedIn && m_receivedInfo && m_receivedPlayerPosition && m_levelReady && !m_disconnected;
}

uint32_t ClientConnection::GetTeleportId() const
{
    return m_teleportId;
}

bool ClientConnection::IsDisconnected() const
{
    return m_disconnected;
}

eDISCONNECT_REASON ClientConnection::GetDisconnectReason() const
{
    return m_disconnectReason;
}

const std::wstring &ClientConnection::GetDisconnectMessage() const
{
    return m_disconnectMessage;
}

uint8_t ClientConnection::GetPlayerIndex() const
{
    return m_playerIndex;
}

std::vector<ClientPlayerListEntry> ClientConnection::GetPlayerList() const
{
    std::vector<ClientPlayerListEntry> entries;

    ClientPlayerListEntry localEntry;
    localEntry.name        = StringUtils::ToUtf16(m_username);
    localEntry.playerIndex = m_playerIndex;
    localEntry.isOperator  = Everstead::GetInstance()->GetLocalPlayer()->GetSharedFlag(eEntityFlag_Operator);
    localEntry.ping        = -1;
    entries.push_back(localEntry);

    for (size_t i = 0; i < m_statusPlayers.size(); i++)
    {
        if (m_statusPlayers[i].playerIndex == m_playerIndex)
        {
            entries[0].ping = m_statusPlayers[i].ping;
            continue;
        }

        ClientPlayerListEntry entry;
        entry.name        = StringUtils::ToUtf16(m_statusPlayers[i].name);
        entry.playerIndex = m_statusPlayers[i].playerIndex;
        entry.isOperator  = m_statusPlayers[i].isOperator;
        entry.ping        = m_statusPlayers[i].ping;
        entries.push_back(entry);
    }

    for (size_t i = 1; i < entries.size(); i++)
    {
        ClientPlayerListEntry current = entries[i];
        size_t j                      = i;
        while (j > 0 && entries[j - 1].playerIndex > current.playerIndex)
        {
            entries[j] = entries[j - 1];
            j--;
        }

        entries[j] = current;
    }

    return entries;
}

const std::string &ClientConnection::GetWorldName() const
{
    return m_worldName;
}

const std::string &ClientConnection::GetServerName() const
{
    return m_serverName;
}

int ClientConnection::GetMaxPlayers() const
{
    return m_maxPlayers;
}

float ClientConnection::GetTps() const
{
    return m_tps;
}

float ClientConnection::GetMspt() const
{
    return m_mspt;
}

RemotePlayer *ClientConnection::FindRemotePlayer(uint8_t playerIndex)
{
    auto it = m_remotePlayers.find(playerIndex);
    if (it == m_remotePlayers.end())
    {
        return nullptr;
    }

    return it->second;
}

Entity *ClientConnection::FindMovedEntity(int entityId)
{
    if (entityId == Everstead::GetInstance()->GetLocalPlayer()->GetId())
    {
        return nullptr;
    }

    return Everstead::GetInstance()->GetLevel()->GetEntityById(entityId);
}

RemotePlayer *ClientConnection::AddRemotePlayer(uint8_t playerIndex, const std::string &username)
{
    std::wstring wideUsername(username.begin(), username.end());
    RemotePlayer *player = new RemotePlayer(Everstead::GetInstance()->GetLevel(), wideUsername);
    player->SetPlayerIndex(playerIndex);
    m_remotePlayers[playerIndex] = player;
    Everstead::GetInstance()->GetLevel()->AddEntity(std::unique_ptr<Entity>(player));
    return player;
}

#endif
