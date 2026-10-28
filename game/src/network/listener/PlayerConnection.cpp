#include "PlayerConnection.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>

#include "../../core/CrashHandler.h"
#include "../../core/EversteadServer.h"
#include "../../core/Localization.h"
#include "../../core/Logger.h"
#include "../../entity/Inventory.h"
#include "../../entity/Mob.h"
#include "../../entity/item/Boat.h"
#include "../../entity/item/LeashKnot.h"
#include "../../entity/item/Minecart.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../entity/player/ServerPlayerGameMode.h"
#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../tag/StringTag.h"
#include "../../utils/Direction.h"
#include "../../utils/StringUtils.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/Block.h"
#include "../../world/block/SignBlock.h"
#include "../../world/blockentity/BlockEntity.h"
#include "../../world/blockentity/SignBlockEntity.h"
#include "../../world/inventory/AnvilContainer.h"
#include "../../world/inventory/ServerContainerMenu.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../Connection.h"
#include "../PlayerList.h"
#include "../command/CommandContext.h"
#include "../command/CommandSuggestions.h"
#include "../packet/Packet.h"
#include "../packet/c2s/ChatC2SPacket.h"
#include "../packet/c2s/DisconnectC2SPacket.h"
#include "../packet/c2s/MovePlayerC2SPacket.h"
#include "../packet/c2s/MoveVehicleC2SPacket.h"
#include "../packet/c2s/PickBlockC2SPacket.h"
#include "../packet/c2s/PlayerActionC2SPacket.h"
#include "../packet/c2s/RenameItemC2SPacket.h"
#include "../packet/c2s/SetCarriedItemC2SPacket.h"
#include "../packet/c2s/SignUpdateC2SPacket.h"
#include "../packet/s2c/AtmosphereS2CPacket.h"
#include "../packet/s2c/ChatS2CPacket.h"
#include "../packet/s2c/DisconnectS2CPacket.h"
#include "../packet/s2c/DynamicLightS2CPacket.h"
#include "../packet/s2c/FogVolumeS2CPacket.h"
#include "../packet/s2c/KeepAliveS2CPacket.h"
#include "../packet/s2c/MovePlayerS2CPacket.h"
#include "../packet/s2c/SetEntityDataS2CPacket.h"
#include "../packet/s2c/SetPlayerPositionS2CPacket.h"
#include "../packet/s2c/SetTimeS2CPacket.h"
#include "../packet/s2c/SignDataS2CPacket.h"
#include "../packet/s2c/SwingS2CPacket.h"
#include "../packet/s2c/TabCompleteS2CPacket.h"

#include <SDL2/SDL.h>
#include <glm/gtx/norm.hpp>

const double PlayerConnection::s_maxMoveDistance        = 20.0;
const double PlayerConnection::s_maxSpectatorSpeedScale = 4.0;
const double PlayerConnection::s_minWorldY              = -512.0;
const double PlayerConnection::s_maxWorldY              = 1024.0;

PlayerConnection::PlayerConnection(EversteadServer *server, Connection *connection, ServerPlayer *player)
{
    m_server              = server;
    m_connection          = connection;
    m_player              = player;
    m_commandDispatcher   = new CommandDispatcher();
    m_chunkSender         = new PlayerChunkSender(player);
    m_lastPacketTime      = std::chrono::steady_clock::now();
    m_ticksSinceKeepAlive = 0;
    m_teleportId          = 0;
    Logger::LogInfo("Player connection created connection=%s playerIndex=%u", m_connection->GetName().c_str(), m_player->GetPlayerIndex());
}

PlayerConnection::~PlayerConnection()
{
    delete m_commandDispatcher;
    delete m_chunkSender;
}

void PlayerConnection::Tick()
{
    m_connection->Tick();
    if (m_connection->IsClosed())
    {
        return;
    }

    bool receivedPacket            = false;
    std::shared_ptr<Packet> packet = m_connection->PollSentPacket();
    while (packet)
    {
        CrashHandler::SetContext("handling packet %d from %s", (int) packet->GetId(), m_player->GetScoreboardName().c_str());
        packet->Handle(this);
        receivedPacket = true;
        packet         = m_connection->PollSentPacket();
    }

    CrashHandler::ClearContext();

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (receivedPacket)
    {
        m_lastPacketTime = now;
    }

    if (now - m_lastPacketTime > std::chrono::seconds(s_timeoutSeconds))
    {
        Logger::LogInfo("Player connection timed out connection=%s playerIndex=%u timeoutSeconds=%d", m_connection->GetName().c_str(),
                        m_player->GetPlayerIndex(), s_timeoutSeconds);
        Disconnect(eDisconnectReason_Timeout);
        return;
    }

    m_ticksSinceKeepAlive++;
    if (m_ticksSinceKeepAlive >= s_keepAliveIntervalTicks)
    {
        m_ticksSinceKeepAlive = 0;
        Send(std::shared_ptr<Packet>(new KeepAliveS2CPacket()));

        Level *playerLevel = m_player->GetLevel();
        if (playerLevel)
        {
            Send(std::shared_ptr<Packet>(new SetTimeS2CPacket(playerLevel->GetWorldProvider().GetWorldTime(),
                                                              playerLevel->GetGameRules().GetBool(eGameRule_DoDaylightCycle))));
        }
    }

    if (!m_server->IsSaving())
    {
        m_chunkSender->Tick();
    }
}

void PlayerConnection::Send(std::shared_ptr<Packet> packet)
{
    ePACKET_ID id = packet->GetId();
    // Block changes wait behind queued chunks because each chunk packet holds a snapshot that would overwrite them
    if (id == ePacketId_ChunkBlocksUpdateS2C || id == ePacketId_BlockUpdateS2C || id == ePacketId_MultiBlockUpdateS2C ||
        id == ePacketId_SpawnerDataS2C || id == ePacketId_SignDataS2C || id == ePacketId_MovingPistonS2C)
    {
        m_connection->QueueDelayedPacket(packet);
    }
    else
    {
        m_connection->QueueIncomingPacket(packet);
    }
}

void PlayerConnection::Disconnect(eDISCONNECT_REASON reason, const std::wstring &text)
{
    std::string reasonText = StringUtils::ToUtf8(Localization::GetInstance()->Get(DisconnectS2CPacket::GetReasonKey(reason)));
    if (text.empty())
    {
        Logger::LogInfo("Disconnecting %s: %s", StringUtils::ToUtf8(m_player->GetName()).c_str(), reasonText.c_str());
    }
    else
    {
        Logger::LogInfo("Disconnecting %s: %s (%s)", StringUtils::ToUtf8(m_player->GetName()).c_str(), reasonText.c_str(),
                        StringUtils::ToUtf8(text).c_str());
    }

    Send(std::shared_ptr<Packet>(new DisconnectS2CPacket(reason, text)));
    m_connection->CloseAfterFlush(reason);
}

bool PlayerConnection::IsServerPacketListener()
{
    return true;
}

void PlayerConnection::OnUnhandledPacket(Packet *packet)
{
    Logger::LogInfo("Player connection unhandled packet connection=%s playerIndex=%u packetId=%d", m_connection->GetName().c_str(),
                    m_player->GetPlayerIndex(), packet->GetId());
}

void PlayerConnection::HandleDisconnect(DisconnectC2SPacket *packet)
{
    Logger::LogInfo("Player connection received disconnect connection=%s playerIndex=%u reason=%u", m_connection->GetName().c_str(),
                    m_player->GetPlayerIndex(), (uint32_t) packet->GetReason());
    Disconnect(packet->GetReason());
}

void PlayerConnection::HandleMovePlayer(MovePlayerC2SPacket *packet)
{
    if (m_server->IsSaving() || m_player->GetHealth() <= 0 || m_player->IsSleeping() || packet->GetTeleportId() != m_teleportId)
    {
        return;
    }

    double x    = packet->GetX();
    double y    = packet->GetY();
    double z    = packet->GetZ();
    float yaw   = packet->GetYaw();
    float pitch = packet->GetPitch();

    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z) || !std::isfinite(yaw) || !std::isfinite(pitch))
    {
        Logger::LogWarn("Rejected non-finite player position");
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    m_player->SetFlying(packet->IsFlying() && GameType::CanFly(m_player->GetGameType()));
    Entity *vehicle = m_player->GetVehicle();
    if (vehicle)
    {
        m_player->SetRotation(yaw, std::clamp(pitch, -90.0f, 90.0f));
        vehicle->PositionRider(m_player);
        return;
    }

    if (y < s_minWorldY || y > s_maxWorldY)
    {
        ResendPosition();
        return;
    }

    glm::dvec3 current = m_player->GetPosition();
    double dx          = x - current.x;
    double dy          = y - current.y;
    double dz          = z - current.z;
    double speedScale  = std::max({1.0, m_player->GetAttributeValue(Attribute::s_movementSpeed) / Attribute::s_movementSpeed.GetDefaultValue(),
                                   m_player->GetAttributeValue(Attribute::s_flyingSpeed) / Attribute::s_flyingSpeed.GetDefaultValue()});
    if (GameType::IsSpectator(m_player->GetGameType()))
    {
        speedScale = std::max(speedScale, s_maxSpectatorSpeedScale);
    }

    double maxDistance = s_maxMoveDistance * speedScale;
    if (dx * dx + dy * dy + dz * dz > maxDistance * maxDistance)
    {
        Logger::LogInfo("Player moved too far, resetting pid index=%u", (uint32_t) m_player->GetPlayerIndex());
        ResendPosition();
        return;
    }

    bool wasOnGround = m_player->IsOnGround();
    m_player->SetPosition(glm::dvec3(x, y, z));
    m_player->SetVelocity(glm::dvec3(dx, dy, dz));
    m_player->SetRotation(yaw, std::clamp(pitch, -90.0f, 90.0f));
    m_player->CauseMovementExhaustion(dx, dy, dz, wasOnGround, packet->IsOnGround());
    m_player->UpdateFallDamage(dy, packet->IsOnGround());
}

void PlayerConnection::HandlePlayerAction(PlayerActionC2SPacket *packet)
{
    if (m_server->IsSaving() || m_player->GetHealth() <= 0)
    {
        return;
    }

    ePLAYER_ACTION action = packet->GetAction();
    if (action == ePlayerAction_StopSleeping)
    {
        m_player->WakeUp();
        return;
    }

    if (action == ePlayerAction_StartSprinting || action == ePlayerAction_StopSprinting || action == ePlayerAction_StartSneaking ||
        action == ePlayerAction_StopSneaking)
    {
        if (action == ePlayerAction_StartSprinting)
        {
            m_player->SetSprinting(true);
        }
        else if (action == ePlayerAction_StopSprinting)
        {
            m_player->SetSprinting(false);
        }
        else if (action == ePlayerAction_StartSneaking)
        {
            m_player->SetSneaking(true);
            if (m_player->GetVehicle())
            {
                m_player->DismountVehicle();
                ResendPosition();
            }
        }
        else
        {
            m_player->SetSneaking(false);
        }

        m_server->GetPlayerList()->SendToOthers(
                m_player, std::shared_ptr<Packet>(new SetEntityDataS2CPacket(m_player->GetPlayerIndex(), m_player->GetSharedFlags())));
        return;
    }

    ServerPlayerGameMode *gameMode = m_player->GetGameMode();
    if (action == ePlayerAction_StartUsingItem)
    {
        gameMode->UseItem();
        return;
    }

    if (action == ePlayerAction_ReleaseUseItem)
    {
        gameMode->ReleaseUsingItem();
        return;
    }

    if (action == ePlayerAction_AbortDestroy)
    {
        gameMode->AbortDestroy();
        return;
    }

    ServerLevel *serverLevel = m_server->GetLevel(m_player->GetLevel()->GetDimensionId());
    if (!serverLevel)
    {
        return;
    }

    const BlockPos &blockPos = packet->GetPos();
    eGAME_TYPE gameType      = m_player->GetGameType();
    if ((!GameType::MayBuild(gameType) && (action == ePlayerAction_Break || action == ePlayerAction_StartDestroy || action == ePlayerAction_Place ||
                                           action == ePlayerAction_UseItem)) ||
        (GameType::IsSpectator(gameType) && action == ePlayerAction_Use))
    {
        gameMode->ResendBlock(serverLevel, blockPos);
        return;
    }

    if (action == ePlayerAction_Break)
    {
        gameMode->DestroyBlock(serverLevel, blockPos);
    }
    else if (action == ePlayerAction_StartDestroy)
    {
        gameMode->StartDestroy(serverLevel, blockPos);
    }
    else if (action == ePlayerAction_Use)
    {
        gameMode->UseBlock(serverLevel, blockPos, Direction::ById(packet->GetPlacedAgainstFace()));
    }
    else if (action == ePlayerAction_Place || action == ePlayerAction_UseItem)
    {
        Direction *face = Direction::ById(packet->GetPlacedAgainstFace());
        if (!face)
        {
            Logger::LogWarn("Rejected player action with face=%d", packet->GetPlacedAgainstFace());
            Disconnect(eDisconnectReason_UnexpectedPacket);
        }
        else if (action == ePlayerAction_Place)
        {
            gameMode->PlaceBlock(serverLevel, blockPos, face, packet->GetData());
        }
        else
        {
            gameMode->UseItemOn(serverLevel, blockPos, face);
        }
    }
    else
    {
        Logger::LogWarn("Rejected player action=%d", (int) action);
        Disconnect(eDisconnectReason_UnexpectedPacket);
    }
}

void PlayerConnection::ResendPosition()
{
    Teleport(m_player->GetPosition(), m_player->GetYaw(), m_player->GetPitch());
}

void PlayerConnection::Teleport(const glm::dvec3 &position, float yaw, float pitch)
{
    m_player->SetPosition(position);
    m_player->SetRotation(yaw, pitch);
    m_player->StoreOld();
    m_teleportId++;
    Send(std::shared_ptr<Packet>(new SetPlayerPositionS2CPacket(position.x, position.y, position.z, yaw, pitch, m_teleportId)));
}

void PlayerConnection::HandleSetCarriedItem(SetCarriedItemC2SPacket *packet)
{
    if (packet->GetSlot() >= Inventory::s_hotbarSize)
    {
        Logger::LogWarn("Rejected carried item slot=%d", (int) packet->GetSlot());
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    m_player->GetInventory()->SetSelectedSlot(packet->GetSlot());
}

void PlayerConnection::HandleContainerClick(ContainerClickC2SPacket *packet)
{
    m_player->GetContainerMenu()->Click(packet);
}

void PlayerConnection::HandlePickBlock(PickBlockC2SPacket *packet)
{
    PlayerList *playerList   = m_server->GetPlayerList();
    eGAME_TYPE gameType      = m_player->GetGameType();
    const BlockPos &pos      = packet->GetPos();
    Level *level             = m_player->GetLevel();
    BlockEntity *blockEntity = level->GetBlockEntity(pos);
    if (packet->GetSlot() >= Inventory::s_hotbarSize || !GameType::HasInfiniteItems(gameType) || GameType::IsSpectator(gameType) ||
        m_player->GetHealth() <= 0 || !m_player->CanReach(pos) || !blockEntity)
    {
        playerList->SendInventoryTo(m_player);
        return;
    }

    Block *block         = Block::ById(level->GetBlockId(pos));
    SignBlock *signBlock = dynamic_cast<SignBlock *>(block);
    Item *item           = Item::ByName((signBlock ? signBlock->GetStandingBlock() : block)->GetResourceLocation());
    if (!item)
    {
        playerList->SendInventoryTo(m_player);
        return;
    }

    std::unique_ptr<CompoundTag> blockEntityTag(new CompoundTag());
    blockEntity->Save(blockEntityTag.get());
    blockEntityTag->Remove("x");
    blockEntityTag->Remove("y");
    blockEntityTag->Remove("z");

    std::unique_ptr<ListTag> lore(new ListTag());
    lore->Add(std::unique_ptr<Tag>(new StringTag("\"(+NBT)\"")));
    std::unique_ptr<CompoundTag> display(new CompoundTag());
    display->Put("Lore", std::move(lore));

    CompoundTag *tag = new CompoundTag();
    tag->Put("BlockEntityTag", std::move(blockEntityTag));
    tag->Put("display", std::move(display));
    ItemInstance stack(item, 1);
    stack.SetTag(tag);
    m_player->GetInventory()->SetPickedItem(packet->GetSlot(), stack);
    playerList->SendInventoryTo(m_player);
}

void PlayerConnection::HandleSignUpdate(SignUpdateC2SPacket *packet)
{
    const BlockPos &pos   = packet->GetPos();
    Level *level          = m_player->GetLevel();
    SignBlockEntity *sign = m_player->CanReach(pos) ? dynamic_cast<SignBlockEntity *>(level->GetBlockEntity(pos)) : nullptr;
    if (!sign || sign->GetEditorId() != m_player->GetId() || sign->IsWaxed())
    {
        return;
    }

    SignBlockEntity::SignText *text = sign->GetText(packet->IsFront());
    for (int i = 0; i < 4; i++)
    {
        std::string line = packet->GetLine(i).substr(0, 384);
        for (size_t found = line.find("\xC2\xA7"); found != std::string::npos; found = line.find("\xC2\xA7"))
        {
            line.erase(found, 2);
        }

        text->messages[i] = line;
    }

    sign->SetEditorId(-1);
    ((ServerLevel *) level)->MarkBlockChunkDirty(pos);
    std::unique_ptr<CompoundTag> tag(new CompoundTag());
    sign->Save(tag.get());
    m_server->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new SignDataS2CPacket(pos, std::move(tag))));
}

void PlayerConnection::HandleChat(ChatC2SPacket *packet)
{
    if (packet->GetMessage().size() > s_maxChatLength)
    {
        Logger::LogWarn("Rejected chat message length=%zu", packet->GetMessage().size());
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    std::string message = StringUtils::Trim(packet->GetMessage());
    if (message.empty())
    {
        return;
    }

    if (message[0] == '/')
    {
        Logger::LogInfo("%s issued server command: %s", StringUtils::ToUtf8(m_player->GetName()).c_str(), message.c_str());
        m_commandDispatcher->Execute(message, m_server, m_player);
        return;
    }

    nlohmann::json component = CommandContext::Translate("chat.type.text");
    component["with"].push_back(m_player->GetDisplayName());
    component["with"].push_back(message);
    m_server->GetPlayerList()->BroadcastChat(component);
}

void PlayerConnection::HandleTabComplete(TabCompleteC2SPacket *packet)
{
    const std::string &text = packet->GetText();
    if (text.size() > s_maxChatLength)
    {
        Logger::LogWarn("Rejected tab complete length=%zu", text.size());
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    std::vector<std::string> completions;
    if (!text.empty() && text[0] == '/')
    {
        m_commandDispatcher->Complete(text, m_server, m_player, packet->HasTarget() ? &packet->GetTarget() : nullptr, &completions);
    }
    else
    {
        std::vector<std::string> words = StringUtils::Split(text, ' ');
        CommandSuggestions(words, &completions).AddPlayers(m_server->GetPlayerList());
    }

    Send(std::shared_ptr<Packet>(new TabCompleteS2CPacket(completions)));
}

void PlayerConnection::HandleSwing(SwingC2SPacket *packet)
{
    (void) packet;
    m_player->OnSwing();
    m_server->GetPlayerList()->SendToOthers(m_player, std::shared_ptr<Packet>(new SwingS2CPacket(m_player->GetPlayerIndex())));
}

void PlayerConnection::HandleDynamicLight(DynamicLightC2SPacket *packet)
{
    if (!m_player->IsOperator())
    {
        return;
    }

    Level *level                 = m_player->GetLevel();
    DynamicLights &dynamicLights = level->GetDynamicLights();
    if (packet->IsRemoved())
    {
        dynamicLights.RemoveLight(packet->GetLight().id);
        m_server->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new DynamicLightS2CPacket(packet->GetLight().id)));
    }
    else if (dynamicLights.SetLight(packet->GetLight()))
    {
        m_server->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new DynamicLightS2CPacket(packet->GetLight())));
    }
}

void PlayerConnection::HandleFogVolume(FogVolumeC2SPacket *packet)
{
    if (!m_player->IsOperator())
    {
        return;
    }

    Level *level                 = m_player->GetLevel();
    DynamicLights &dynamicLights = level->GetDynamicLights();
    if (packet->IsRemoved())
    {
        dynamicLights.RemoveFogVolume(packet->GetFogVolume().id);
        m_server->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new FogVolumeS2CPacket(packet->GetFogVolume().id)));
    }
    else if (dynamicLights.SetFogVolume(packet->GetFogVolume()))
    {
        m_server->GetPlayerList()->SendToLevel(level, std::shared_ptr<Packet>(new FogVolumeS2CPacket(packet->GetFogVolume())));
    }
}

void PlayerConnection::HandleAtmosphere(AtmosphereC2SPacket *packet)
{
    if (!m_player->IsOperator())
    {
        return;
    }

    m_player->GetLevel()->GetDynamicLights().SetAtmosphere(packet->GetAirDensity(), packet->GetAnisotropy());
    m_server->GetPlayerList()->SendToLevel(m_player->GetLevel(),
                                           std::shared_ptr<Packet>(new AtmosphereS2CPacket(packet->GetAirDensity(), packet->GetAnisotropy())));
}

void PlayerConnection::HandleRespawn(RespawnC2SPacket *packet)
{
    (void) packet;
    if (m_player->GetHealth() > 0)
    {
        return;
    }

    m_player->Respawn();
}

void PlayerConnection::HandleChunkAck(ChunkAckC2SPacket *packet)
{
    if (!m_chunkSender->Acknowledge(packet->GetCount(), packet->GetRenderDistance()))
    {
        Disconnect(eDisconnectReason_UnexpectedPacket);
    }
}

void PlayerConnection::HandleAttack(AttackC2SPacket *packet)
{
    if (m_player->GetVehicle() && m_player->GetVehicle()->GetId() == packet->GetEntityId())
    {
        return;
    }

    LeashKnot *knot = dynamic_cast<LeashKnot *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (knot)
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 &&
            glm::distance2(m_player->GetPosition(), knot->GetPosition()) < 6.0 * 6.0)
        {
            knot->Hurt();
        }

        return;
    }

    Boat *boat = dynamic_cast<Boat *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (boat)
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 &&
            glm::distance2(m_player->GetPosition(), boat->GetPosition()) < 6.0 * 6.0)
        {
            ItemInstance *held = m_player->GetInventory()->GetSelectedItem();
            boat->Hurt((int) m_player->GetAttributeValue(Attribute::s_attackDamage) + (held->IsEmpty() ? 0 : held->GetItem()->GetAttackDamage()),
                       m_player);
        }

        return;
    }

    Minecart *minecart = dynamic_cast<Minecart *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (minecart)
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 &&
            glm::distance2(m_player->GetPosition(), minecart->GetPosition()) < 6.0 * 6.0)
        {
            ItemInstance *held = m_player->GetInventory()->GetSelectedItem();
            minecart->Hurt((int) m_player->GetAttributeValue(Attribute::s_attackDamage) + (held->IsEmpty() ? 0 : held->GetItem()->GetAttackDamage()),
                           m_player);
        }

        return;
    }

    LivingEntity *target = m_server->GetPlayerList()->GetPlayerById(packet->GetEntityId());
    if (!target)
    {
        target = dynamic_cast<LivingEntity *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    }

    if (!target || target == m_player || target->GetLevel() != m_player->GetLevel() || GameType::IsSpectator(m_player->GetGameType()) ||
        m_player->GetHealth() <= 0 || target->GetHealth() <= 0)
    {
        return;
    }

    glm::dvec3 eyeOffset(0.0, m_player->GetEyeHeight(), 0.0);
    glm::dvec3 eyePosition = m_player->GetPosition() + eyeOffset;
    glm::dvec3 toTarget    = target->GetPosition() + eyeOffset - eyePosition;
    HitResult *hit         = m_player->GetLevel()->Clip(eyePosition, toTarget, (float) glm::length(toTarget), false);
    double maxDistance     = hit->IsMiss() ? 6.0 : 3.0;
    delete hit;
    if (glm::distance2(m_player->GetPosition(), target->GetPosition()) >= maxDistance * maxDistance)
    {
        return;
    }

    m_player->Attack(target);
}

void PlayerConnection::HandleInteract(InteractC2SPacket *packet)
{
    LeashKnot *knot = dynamic_cast<LeashKnot *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (knot)
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 &&
            glm::distance2(m_player->GetPosition(), knot->GetPosition()) < 6.0 * 6.0)
        {
            knot->Interact(m_player);
        }

        return;
    }

    Boat *boat = dynamic_cast<Boat *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (boat)
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 && !m_player->IsSneaking() && !m_player->GetVehicle() &&
            glm::distance2(m_player->GetPosition(), boat->GetPosition()) < 6.0 * 6.0 && m_player->StartRiding(boat))
        {
            m_player->GetLevel()->OnPassengersChanged(boat);
        }

        return;
    }

    Minecart *minecart = dynamic_cast<Minecart *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (minecart)
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 &&
            glm::distance2(m_player->GetPosition(), minecart->GetPosition()) < 6.0 * 6.0 && minecart->Interact(m_player))
        {
            m_server->GetPlayerList()->SendInventoryTo(m_player);
        }

        return;
    }

    Mob *target = dynamic_cast<Mob *>(m_player->GetLevel()->GetEntityById(packet->GetEntityId()));
    if (!target || GameType::IsSpectator(m_player->GetGameType()) || m_player->GetHealth() <= 0 || target->GetHealth() <= 0 ||
        glm::distance2(m_player->GetPosition(), target->GetPosition()) >= 6.0 * 6.0)
    {
        return;
    }

    if (target->InteractLeash(m_player) || target->Interact(m_player))
    {
        m_server->GetPlayerList()->SendInventoryTo(m_player);
    }
}

void PlayerConnection::HandleMoveVehicle(MoveVehicleC2SPacket *packet)
{
    Boat *vehicle = dynamic_cast<Boat *>(m_player->GetVehicle());
    if (!vehicle || vehicle->GetPassengers()[0] != m_player)
    {
        return;
    }

    glm::dvec3 position(packet->GetX(), packet->GetY(), packet->GetZ());
    if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z) || !std::isfinite(packet->GetYaw()))
    {
        Logger::LogWarn("Rejected non-finite vehicle position");
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    if (glm::distance2(position, vehicle->GetPosition()) > 10.0 * 10.0)
    {
        return;
    }

    vehicle->SetPosition(position);
    vehicle->SetRotation(packet->GetYaw(), vehicle->GetPitch());
    vehicle->SetPaddleState(packet->GetLeftPaddle(), packet->GetRightPaddle());
    vehicle->PositionRider(m_player);
}

void PlayerConnection::HandlePlayerInput(PlayerInputC2SPacket *packet)
{
    if (!std::isfinite(packet->GetForward()))
    {
        Disconnect(eDisconnectReason_UnexpectedPacket);
        return;
    }

    if (dynamic_cast<Minecart *>(m_player->GetVehicle()))
    {
        m_player->SetMoveInputs(std::clamp(packet->GetForward(), -1.0f, 1.0f), 0.0f);
    }
}

void PlayerConnection::HandleRenameItem(RenameItemC2SPacket *packet)
{
    const std::string &raw = packet->GetName();
    std::string name;
    int length = 0;
    for (size_t i = 0; i < raw.size(); i++)
    {
        unsigned char c = (unsigned char) raw[i];
        if (c < 32 || c == 127 || (c == 0xC2 && i + 1 < raw.size() && (unsigned char) raw[i + 1] == 0xA7))
        {
            i += c == 0xC2 ? 1 : 0;
            continue;
        }

        if ((c & 0xC0) != 0x80)
        {
            length++;
        }

        name += (char) c;
    }

    if (length <= AnvilContainer::s_maxNameLength)
    {
        m_player->GetContainerMenu()->RenameAnvilItem(name);
    }
}

Connection *PlayerConnection::GetConnection() const
{
    return m_connection;
}

ServerPlayer *PlayerConnection::GetPlayer() const
{
    return m_player;
}

PlayerChunkSender *PlayerConnection::GetChunkSender() const
{
    return m_chunkSender;
}
