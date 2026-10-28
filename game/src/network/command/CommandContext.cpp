#include "CommandContext.h"

#include "../../core/Localization.h"
#include "../../core/Logger.h"
#include "../../core/ResourceLocation.h"
#include "../../entity/LivingEntity.h"
#include "../../ui/ChatMessageParser.h"
#include "../../utils/StringUtils.h"
#include "../../world/block/BlockRegistry.h"
#include "../listener/PlayerConnection.h"
#include "../packet/s2c/ChatS2CPacket.h"

#include <glm/trigonometric.hpp>

CommandContext::CommandContext(EversteadServer *server, ServerPlayer *source)
{
    m_server         = server;
    m_source         = source;
    m_entity         = source;
    m_position       = source->GetPosition();
    m_yaw            = source->GetYaw();
    m_pitch          = source->GetPitch();
    m_level          = server->GetLevel(source->GetLevel()->GetDimensionId());
    m_anchoredAtEyes = false;
    m_adminCommand   = false;
    m_failed         = false;
    m_blockChanges   = {};
}

CommandContext::~CommandContext()
{}

EversteadServer *CommandContext::GetServer() const
{
    return m_server;
}

PlayerList *CommandContext::GetPlayerList() const
{
    return m_server->GetPlayerList();
}

ServerPlayer *CommandContext::GetCaller() const
{
    return m_source;
}

Entity *CommandContext::GetEntity() const
{
    return m_entity;
}

void CommandContext::SetEntity(Entity *entity)
{
    m_entity = entity;
}

ServerPlayer *CommandContext::GetPlayer() const
{
    return dynamic_cast<ServerPlayer *>(m_entity);
}

bool CommandContext::RequirePlayer(ServerPlayer **player) const
{
    *player = GetPlayer();
    if (!*player)
    {
        SendSystemMessage(Translate("permissions.requires.player"), true);
        return false;
    }

    return true;
}

const glm::dvec3 &CommandContext::GetPosition() const
{
    return m_position;
}

void CommandContext::SetPosition(const glm::dvec3 &position)
{
    m_position = position;
}

glm::dvec3 CommandContext::GetAnchorPosition() const
{
    LivingEntity *living = dynamic_cast<LivingEntity *>(m_entity);
    if (m_anchoredAtEyes && living)
    {
        return m_position + glm::dvec3(0.0, living->GetEyeHeight(), 0.0);
    }

    return m_position;
}

void CommandContext::SetAnchoredAtEyes(bool anchoredAtEyes)
{
    m_anchoredAtEyes = anchoredAtEyes;
}

float CommandContext::GetYaw() const
{
    return m_yaw;
}

float CommandContext::GetPitch() const
{
    return m_pitch;
}

void CommandContext::SetRotation(float yaw, float pitch)
{
    m_yaw   = yaw;
    m_pitch = pitch;
}

void CommandContext::FacePosition(const glm::dvec3 &target)
{
    glm::dvec3 delta  = target - GetAnchorPosition();
    double horizontal = sqrt(delta.x * delta.x + delta.z * delta.z);
    m_yaw             = (float) glm::degrees(atan2(delta.z, delta.x));
    m_pitch           = (float) glm::degrees(atan2(delta.y, horizontal));
}

glm::dvec3 CommandContext::GetFront() const
{
    double yawRad   = glm::radians((double) m_yaw);
    double pitchRad = glm::radians((double) m_pitch);
    return glm::dvec3(cos(yawRad) * cos(pitchRad), sin(pitchRad), sin(yawRad) * cos(pitchRad));
}

void CommandContext::SetAdminCommand(bool adminCommand)
{
    m_adminCommand = adminCommand;
}

ServerLevel *CommandContext::GetLevel() const
{
    return m_level;
}

void CommandContext::SetLevel(ServerLevel *level)
{
    m_level = level;
}

bool CommandContext::HasFailed() const
{
    return m_failed;
}

Block *CommandContext::FindBlock(const std::string &name) const
{
    ResourceLocation location;
    if (!ResourceLocation::TryParse(StringUtils::ToLower(name), &location))
    {
        return nullptr;
    }

    return Block::ByName(location);
}

Item *CommandContext::FindItem(const std::string &name) const
{
    ResourceLocation location;
    if (!ResourceLocation::TryParse(StringUtils::ToLower(name), &location))
    {
        return nullptr;
    }

    return Item::ByName(location);
}

void CommandContext::SetBlock(const BlockPos &pos, Block *block, uint16_t data)
{
    Level *level = GetLevel();
    level->SetBlock(pos, block, nullptr, data);
    m_blockChanges.push_back({pos, (uint16_t) BlockRegistry::GetInstance()->IdOf(block), data});
}

void CommandContext::FlushBlockChanges()
{
    if (m_blockChanges.empty())
    {
        return;
    }

    m_server->GetPlayerList()->SendToLevel(GetLevel(), std::shared_ptr<Packet>(new MultiBlockUpdateS2CPacket(m_blockChanges)));
    m_blockChanges.clear();
}

nlohmann::json CommandContext::Translate(const std::string &key, const std::vector<std::wstring> &arguments)
{
    nlohmann::json component;
    component["translate"] = key;
    component["with"]      = nlohmann::json::array();
    for (size_t i = 0; i < arguments.size(); i++)
    {
        component["with"].push_back(StringUtils::ToUtf8(arguments[i]));
    }

    return component;
}

void CommandContext::SendSystemMessage(const nlohmann::json &message, bool error) const
{
    if (error)
    {
        m_failed                 = true;
        nlohmann::json component = message;
        component["color"]       = "red";
        SendChatMessage(m_source, component.dump(), true);
        return;
    }

    GameRules &gameRules = GetLevel()->GetGameRules();
    if (m_adminCommand && gameRules.GetBool(eGameRule_LogAdminCommands))
    {
        Logger::LogInfo("[%s: %s]", StringUtils::ToUtf8(GetPlayerDisplayName(m_source)).c_str(), ChatMessageParser().ToPlainText(message).c_str());
    }

    if (!gameRules.GetBool(eGameRule_SendCommandFeedback))
    {
        return;
    }

    SendChatMessage(m_source, message.dump(), true);
    if (!m_adminCommand)
    {
        return;
    }

    nlohmann::json component = Translate("chat.type.admin", {GetPlayerDisplayName(m_source)});
    component["with"].push_back(message);
    component["color"]                                        = "gray";
    component["italic"]                                       = true;
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        if (players[i].get() != m_source && players[i]->IsOperator())
        {
            SendChatMessage(players[i].get(), component.dump(), true);
        }
    }
}

void CommandContext::SendChatMessage(ServerPlayer *target, const std::string &message, bool advanced) const
{
    SendPacket(target, std::shared_ptr<Packet>(new ChatS2CPacket(message, advanced)));
}

void CommandContext::SendPacket(ServerPlayer *target, std::shared_ptr<Packet> packet) const
{
    target->GetPlayerConnection()->Send(packet);
}

void CommandContext::TeleportPlayer(ServerPlayer *target, const glm::dvec3 &position, float yaw, float pitch) const
{
    target->GetPlayerConnection()->Teleport(position, yaw, pitch);
}

void CommandContext::BanPlayer(ServerPlayer *target) const
{
    PlayerList *playerList = m_server->GetPlayerList();
    playerList->AddBanned(target->GetUniqueID(), StringUtils::ToUtf8(target->GetName()));

    nlohmann::json nameComponent;
    nameComponent["text"] = StringUtils::ToUtf8(GetPlayerDisplayName(target));

    nlohmann::json component;
    component["translate"] = "multiplayer.player.banned";
    component["color"]     = "yellow";
    component["with"]      = nlohmann::json::array();
    component["with"].push_back(nameComponent);
    playerList->BroadcastChat(component);

    target->GetPlayerConnection()->Disconnect(eDisconnectReason_Banned);
}

std::wstring CommandContext::GetPlayerDisplayName(ServerPlayer *player) const
{
    if (player->GetName().empty())
    {
        return Localization::GetInstance()->Get("friends.unknown_player");
    }

    return player->GetName();
}

std::wstring CommandContext::JoinPlayerNames(const std::vector<ServerPlayer *> &players) const
{
    std::wstring names;
    for (size_t i = 0; i < players.size(); i++)
    {
        if (i > 0)
        {
            names += L", ";
        }

        names += GetPlayerDisplayName(players[i]);
    }

    return names;
}
