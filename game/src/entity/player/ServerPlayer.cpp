#include "ServerPlayer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../core/EversteadServer.h"
#include "../../core/GameInfo.h"
#include "../../network/PlayerList.h"
#include "../../network/command/CommandContext.h"
#include "../../network/listener/PlayerConnection.h"
#include "../../network/packet/s2c/AnimateCritS2CPacket.h"
#include "../../network/packet/s2c/ChatS2CPacket.h"
#include "../../network/packet/s2c/KnockbackS2CPacket.h"
#include "../../network/packet/s2c/PlayerDeathS2CPacket.h"
#include "../../network/packet/s2c/PlayerSleepS2CPacket.h"
#include "../../network/packet/s2c/SetEquipmentS2CPacket.h"
#include "../../network/packet/s2c/SetExperienceS2CPacket.h"
#include "../../network/packet/s2c/SetHealthS2CPacket.h"
#include "../../utils/StringUtils.h"
#include "../../utils/math/Mth.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../../world/WorldBorder.h"
#include "../../world/block/BedBlock.h"
#include "../../world/block/Block.h"
#include "../../world/block/BrittleIceBlock.h"
#include "../../world/block/FarmlandBlock.h"
#include "../../world/block/FractureCoreBlock.h"
#include "../../world/block/PointedDripstoneBlock.h"
#include "../../world/block/PressurePlateBlock.h"
#include "../../world/block/WeightedPressurePlateBlock.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/fracture/FractureTraits.h"
#include "../../world/inventory/ServerContainerMenu.h"
#include "../../world/item/FoodItem.h"
#include "../../world/item/Item.h"
#include "../item/ExperienceOrb.h"
#include "../item/ItemEntity.h"
#include "../monster/Monster.h"
#include "ServerPlayerGameMode.h"

#include <nlohmann/json.hpp>

ServerPlayer::ServerPlayer(EversteadServer *server, Level *level, const std::wstring &name, uint8_t playerIndex) : Player(level, name)
{
    m_server                  = server;
    m_pid                     = 0;
    m_connection              = nullptr;
    m_gameMode                = new ServerPlayerGameMode(this);
    m_containerMenu           = new ServerContainerMenu(this);
    m_groundContact           = false;
    m_lastSentHealth          = -1;
    m_lastSentFoodLevel       = -1;
    m_lastSentSaturationLevel = -1.0f;
    m_experienceCooldown      = 0;
    m_lastLevelUpSoundTick    = -100;
    m_inTear                  = false;
    m_swingsSinceHit          = 0;
    SetPlayerIndex(playerIndex);
}

ServerPlayer::~ServerPlayer()
{
    delete m_connection;
    delete m_gameMode;
    delete m_containerMenu;
}

eENTITY_TYPE ServerPlayer::GetType()
{
    return s_type;
}

void ServerPlayer::Tick()
{
    if (m_connection)
    {
        m_connection->Tick();
        if (!m_connection->GetConnection()->IsClosed())
        {
            m_gameMode->Tick();
            m_containerMenu->Tick();
        }
    }

    m_totalTickCount++;
    if (m_experienceCooldown > 0)
    {
        m_experienceCooldown--;
    }

    TickHurt();
    TickFire();
    TickCramming();
    TickSteppedBlocks();
    TickTear();
    TickFractures();
    if (TickAirSupply())
    {
        Hurt(eDamageSource_Drown, 2, nullptr);
    }

    TickSleeping();
    if (m_sleeping)
    {
        BedBlock *bed = dynamic_cast<BedBlock *>(Block::ById(m_level->GetBlockId(m_bedPos)));
        if (!bed || m_level->GetSkyDarken() < 4)
        {
            WakeUp();
        }
    }

    TickUsingItem();
    if (IsUsingItem() != GetSharedFlag(eEntityFlag_UsingItem))
    {
        SetSharedFlag(eEntityFlag_UsingItem, IsUsingItem());
        m_server->GetPlayerList()->BroadcastPlayerFlags(this);
    }

    if (m_level->GetDifficulty() == 0 && m_level->GetGameRules().GetBool(eGameRule_NaturalRegeneration))
    {
        if (m_health < GetMaxHealth() && m_totalTickCount % 20 == 0)
        {
            Heal(1);
        }

        if (m_foodData.GetSaturationLevel() < 20.0f && m_totalTickCount % 20 == 0)
        {
            m_foodData.SetSaturationLevel(m_foodData.GetSaturationLevel() + 1.0f);
        }

        if (m_foodData.NeedsFood() && m_totalTickCount % 10 == 0)
        {
            m_foodData.SetFoodLevel(m_foodData.GetFoodLevel() + 1);
        }
    }

    if (GameType::CanHurtPlayer(GetGameType()))
    {
        m_foodData.Tick(this);
    }

    if (m_health != m_lastSentHealth || m_foodData.GetFoodLevel() != m_lastSentFoodLevel ||
        m_foodData.GetSaturationLevel() != m_lastSentSaturationLevel)
    {
        SendHealth();
    }

    ItemInstance *heldItem = m_inventory.GetSelectedItem();
    if (heldItem->GetItem() != m_lastSentHeldItem.GetItem() || heldItem->GetAuxValue() != m_lastSentHeldItem.GetAuxValue() ||
        heldItem->IsEmpty() != m_lastSentHeldItem.IsEmpty())
    {
        m_lastSentHeldItem = *heldItem;
        m_server->GetPlayerList()->SendToOthers(this, std::shared_ptr<Packet>(new SetEquipmentS2CPacket(GetPlayerIndex(), 0, *heldItem)));
    }

    if (m_position.y < -64.0)
    {
        Hurt(eDamageSource_OutOfWorld, 4, nullptr);
    }

    WorldBorder &border = m_level->GetWorldBorder();
    double outside      = border.GetDistanceToBorder(m_position.x, m_position.z) + border.GetDamageBuffer();
    if (outside < 0.0 && border.GetDamagePerBlock() > 0.0)
    {
        Hurt(eDamageSource_OutsideBorder, std::max(1, (int) floor(-outside * border.GetDamagePerBlock())), nullptr);
    }
}

void ServerPlayer::UpdateFallDamage(double moveY, bool onGround)
{
    m_onGround      = onGround;
    m_groundContact = m_groundContact || onGround;
    UpdateAABB();
    if (m_flying || GameType::IsSpectator(GetGameType()) || IsInWater() || IsOnLadder())
    {
        m_fallDistance = 0.0f;
        return;
    }

    if (!onGround)
    {
        if (moveY < 0.0)
        {
            m_fallDistance -= (float) moveY;
        }
        return;
    }

    float fallDistance = m_fallDistance;
    m_fallDistance     = 0.0f;
    if (GameType::CanHurtPlayer(GetGameType()))
    {
        CauseFallDamage(fallDistance);
    }
}

void ServerPlayer::CauseMovementExhaustion(double dx, double dy, double dz, bool wasOnGround, bool onGround)
{
    if (wasOnGround && !onGround && dy > 0.0)
    {
        CauseFoodExhaustion(IsSprinting() ? FoodData::kExhaustionSprintJump : FoodData::kExhaustionJump);
    }

    float horizontal = (float) round(sqrt(dx * dx + dz * dz) * 100.0) * 0.01f;
    if (IsInWater() && m_level->IsLiquidAt(m_position + glm::dvec3(0.0, 1.62, 0.0), false))
    {
        CauseFoodExhaustion(FoodData::kExhaustionSwim * (float) round(sqrt(dx * dx + dy * dy + dz * dz) * 100.0) * 0.01f);
    }
    else if (IsInWater())
    {
        CauseFoodExhaustion(FoodData::kExhaustionSwim * horizontal);
    }
    else if (!IsOnLadder() && onGround && IsSprinting())
    {
        CauseFoodExhaustion(FoodData::kExhaustionSprint * horizontal);
    }
}

bool ServerPlayer::CanReach(const BlockPos &pos) const
{
    constexpr double kReachTolerance = 1.0;

    glm::dvec3 eyePosition = m_position + glm::dvec3(0.0, 1.62, 0.0);
    double deltaX          = (double) pos.x + 0.5 - eyePosition.x;
    double deltaY          = (double) pos.y + 0.5 - eyePosition.y;
    double deltaZ          = (double) pos.z + 0.5 - eyePosition.z;
    double maxDistance     = GetBlockReach() + kReachTolerance;
    return deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ <= maxDistance * maxDistance;
}

// This latches landings shorter than the tracker interval so other clients still see them
bool ServerPlayer::ConsumeGroundContact()
{
    bool contact    = m_groundContact;
    m_groundContact = m_onGround;
    return contact;
}

float ServerPlayer::GetHeadYaw() const
{
    return m_yaw;
}

void ServerPlayer::Heal(int amount)
{
    if (m_health <= 0)
    {
        return;
    }

    LivingEntity::Heal(amount);
    SendHealth();
}

bool ServerPlayer::Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker)
{
    if (source != eDamageSource_OutOfWorld && !GameType::CanHurtPlayer(GetGameType()))
    {
        return false;
    }

    PlayerTeam *team = GetTeam();
    if (dynamic_cast<Player *>(attacker) && team && team == attacker->GetTeam() && !team->friendlyFire)
    {
        return false;
    }

    if (source == eDamageSource_Mob || source == eDamageSource_Explosion)
    {
        int difficulty = m_level->GetDifficulty();
        if (difficulty == 0)
        {
            return false;
        }

        if (difficulty == 1)
        {
            damage = damage / 2 + 1;
        }
        else if (difficulty == 3)
        {
            damage = damage * 3 / 2;
        }
    }

    std::vector<std::pair<FractureTrait *, int>> fractures = GetArmorFractures();
    for (size_t i = 0; i < fractures.size(); i++)
    {
        if (fractures[i].first->PreventHurt(this, source, fractures[i].second))
        {
            return false;
        }
    }

    if (!LivingEntity::Hurt(source, damage, attacker))
    {
        return false;
    }

    for (size_t i = 0; i < fractures.size(); i++)
    {
        fractures[i].first->OnHurt(this, fractures[i].second);
    }

    return true;
}

void ServerPlayer::OnHurt(eDAMAGE_SOURCE source)
{
    CauseDamageExhaustion(source);
    SendHealth();
}

int ServerPlayer::AbsorbDamage(eDAMAGE_SOURCE source, int damage)
{
    if (!IsArmorDamage(source))
    {
        return damage;
    }

    if (IsBlocking())
    {
        damage = (damage + 1) >> 1;
    }

    int absorbed         = LivingEntity::AbsorbDamage(source, damage);
    int wear             = std::max(1, damage / 4);
    bool damaged         = false;
    Inventory *inventory = GetInventory();
    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        ItemInstance *armor = inventory->GetArmor(i);
        if (armor->IsEmpty() || armor->GetItem()->GetArmorSlot() < 0)
        {
            continue;
        }

        damaged = true;
        armor->HurtAndBreak(wear, this);
        if (armor->IsEmpty())
        {
            *armor = ItemInstance();
            SendArmor(i);
        }
    }

    if (damaged)
    {
        m_server->GetPlayerList()->SendInventoryTo(this);
    }

    return absorbed;
}

void ServerPlayer::SendArmor(int slot)
{
    m_server->GetPlayerList()->SendToOthers(
            this, std::shared_ptr<Packet>(new SetEquipmentS2CPacket(GetPlayerIndex(), (uint8_t) (slot + 1), *GetInventory()->GetArmor(slot))));
}

void ServerPlayer::Die(eDAMAGE_SOURCE source, LivingEntity *attacker)
{
    constexpr const char *kCauses[] = {
            "generic",           "inFire",    "onFire",        "lava",   "fall",     "outOfWorld",     "player", "stalagmite",
            "fallingStalactite", "explosion", "outsideBorder", "starve", "drown",    "thrown",         "mob",    "arrow",
            "lightningBolt",     "hotFloor",  "pricklyVine",   "cactus", "cramming", "sweetBerryBush", "rift",   "anvil"};

    m_containerMenu->CloseAll();
    DismountVehicle();

    GameRules &gameRules = m_level->GetGameRules();
    if (!gameRules.GetBool(eGameRule_KeepInventory))
    {
        Inventory *inventory = GetInventory();
        for (int i = 0; i < Inventory::s_slotCount; i++)
        {
            DropItem(*inventory->GetSlot(i));
            inventory->SetSlot(i, ItemInstance());
        }

        DropItem(inventory->GetCarried());
        inventory->SetCarried(ItemInstance());
        m_server->GetPlayerList()->SendInventoryTo(this);
        for (int i = 0; i < Inventory::s_armorSize; i++)
        {
            SendArmor(i);
        }

        ExperienceOrb::SpawnOrbs(m_level, m_position, std::min(m_experienceLevel * 7, 100));
        SetExperience(0.0f, 0, 0);
        SendExperience();
    }

    LivingEntity::Die(source, attacker);

    nlohmann::json component;
    component["translate"] = std::string("death.attack.") + kCauses[source];
    component["with"]      = nlohmann::json::array();
    component["with"].push_back(GetDisplayName());
    if (attacker)
    {
        component["with"].push_back(attacker->GetDisplayName());
    }

    PlayerTeam *team                                          = GetTeam();
    eTEAM_VISIBILITY visibility                               = team ? team->deathMessageVisibility : eTeamVisibility_Always;
    std::shared_ptr<Packet> deathMessage                      = std::shared_ptr<Packet>(new PlayerDeathS2CPacket(GetPlayerIndex(), component.dump()));
    std::shared_ptr<Packet> silentDeath                       = std::shared_ptr<Packet>(new PlayerDeathS2CPacket(GetPlayerIndex(), ""));
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_server->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        bool allied = team && players[i]->GetTeam() == team;
        bool shown  = !kStickFight &&
                      (players[i].get() == this || visibility == eTeamVisibility_Always ||
                       (visibility == eTeamVisibility_HideForOtherTeams && allied) || (visibility == eTeamVisibility_HideForOwnTeam && !allied));
        players[i]->GetPlayerConnection()->Send(shown ? deathMessage : silentDeath);
    }
    if (gameRules.GetBool(eGameRule_DoImmediateRespawn))
    {
        Respawn();
    }
}

void ServerPlayer::DropItem(const ItemInstance &item)
{
    if (item.IsEmpty())
    {
        return;
    }

    double speed = m_level->GetRandom().NextDouble() * 0.5;
    double angle = m_level->GetRandom().NextDouble() * std::numbers::pi * 2.0;
    std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
    itemEntity->Sync(m_position + glm::dvec3(0.0, GetEyeHeight() - 0.3, 0.0), glm::dvec3(-sin(angle) * speed, 0.2, cos(angle) * speed), item, 0, 40);
    m_level->AddEntity(std::move(itemEntity));
}

ItemEntity *ServerPlayer::ThrowItem(const ItemInstance &item)
{
    if (item.IsEmpty())
    {
        return nullptr;
    }

    const double speed     = 0.3;
    const double lift      = 0.1;
    const double spread    = 0.02;
    const double eyeOffset = 1.62 - 0.3;
    const int pickupDelay  = 40;

    float yaw   = m_yaw * (float) std::numbers::pi / 180.0f;
    float pitch = m_pitch * (float) std::numbers::pi / 180.0f;
    glm::dvec3 front(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));

    double angle     = m_level->GetRandom().NextDouble() * std::numbers::pi * 2.0;
    double magnitude = m_level->GetRandom().NextDouble() * spread;
    glm::dvec3 velocity(front.x * speed + cos(angle) * magnitude,
                        front.y * speed + lift + (m_level->GetRandom().NextDouble() - m_level->GetRandom().NextDouble()) * 0.1,
                        front.z * speed + sin(angle) * magnitude);
    std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
    itemEntity->Sync(m_position + glm::dvec3(0.0, eyeOffset, 0.0), velocity, item, 0, pickupDelay);
    ItemEntity *thrown = itemEntity.get();
    m_level->AddEntity(std::move(itemEntity));
    return thrown;
}

void ServerPlayer::Respawn()
{
    SetHealth(GetMaxHealth());
    m_lastHealth       = m_health;
    m_invulnerableTime = 0;
    m_hurtTime         = 0;
    m_fireTicks        = 0;
    m_airSupply        = s_totalAirSupply;
    m_fallDistance     = 0.0f;
    m_foodData         = FoodData();
    StopUsingItem();
    if (kStickFight)
    {
        m_server->GetStickFight()->OnRespawn(this);
        m_server->GetPlayerList()->SendInventoryTo(this);
    }

    TeleportToSpawn(HasRespawnPosition() ? m_respawnDimension : eDimensionId_Overworld);
    SendHealth();
    m_server->GetPlayerList()->RespawnPlayer(this);
}

void ServerPlayer::TeleportToSpawn(eDIMENSION_ID dimension)
{
    ServerLevel *level = m_server->GetOrCreateLevel(dimension);
    glm::dvec3 spawn   = level->FindPlayerSpawn(level->GetWorldSpawn());
    if (m_level != level)
    {
        m_server->GetPlayerList()->ChangeDimension(this, dimension, spawn, 1.0f);
    }

    if (HasRespawnPosition() && m_respawnDimension == dimension)
    {
        for (int x = Mth::FloorDiv(m_respawnPosition.x - 2, Chunk::s_sizeX); x <= Mth::FloorDiv(m_respawnPosition.x + 2, Chunk::s_sizeX); x++)
        {
            for (int z = Mth::FloorDiv(m_respawnPosition.z - 2, Chunk::s_sizeZ); z <= Mth::FloorDiv(m_respawnPosition.z + 2, Chunk::s_sizeZ); z++)
            {
                m_level->EnsureChunkLoaded(ChunkPos(x, z));
            }
        }

        BlockPos standUp = m_respawnPosition;
        BedBlock *bed    = dynamic_cast<BedBlock *>(Block::ById(m_level->GetBlockId(m_respawnPosition)));
        if (bed ? bed->FindStandUpPosition(m_level, m_respawnPosition, &standUp) : m_respawnForced)
        {
            spawn = glm::dvec3(standUp.x + 0.5, standUp.y, standUp.z + 0.5);
        }
        else
        {
            ClearRespawnPosition();
            SendStatusMessage("tile.bed.notValid");
        }
    }

    m_connection->Teleport(spawn, m_yaw, 0.0f);
}

void ServerPlayer::TrySleep(const BlockPos &pos)
{
    BedBlock *bed = (BedBlock *) Block::ById(m_level->GetBlockId(pos));
    uint16_t data = m_level->GetBlockData(pos);
    BlockPos head = bed->GetHeadPos(pos, data);
    if (m_sleeping || m_health <= 0)
    {
        return;
    }

    const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
    if (bed->IsOccupied(data))
    {
        for (size_t i = 0; i < entities.size(); i++)
        {
            Player *player = dynamic_cast<Player *>(entities[i].get());
            if (player && player->IsSleeping() && player->GetBedPos() == head)
            {
                SendStatusMessage("tile.bed.occupied");
                return;
            }
        }

        bed->SetOccupied(m_level, head, false);
    }

    if (!kStickFight && (!HasRespawnPosition() || m_respawnPosition != head))
    {
        SetRespawnPosition(head, m_level->GetDimensionId(), false);
        SendStatusMessage("tile.bed.setSpawn");
    }

    if (m_level->GetSkyDarken() < 4)
    {
        SendStatusMessage("tile.bed.noSleep");
        return;
    }

    for (size_t i = 0; i < entities.size(); i++)
    {
        glm::dvec3 position = entities[i]->GetPosition();
        if (dynamic_cast<Monster *>(entities[i].get()) && fabs(position.x - (head.x + 0.5)) < 8.0 && fabs(position.y - head.y) < 5.0 &&
            fabs(position.z - (head.z + 0.5)) < 8.0)
        {
            SendStatusMessage("tile.bed.notSafe");
            return;
        }
    }

    Direction *direction = bed->GetHeadDirection(data);
    bed->SetOccupied(m_level, head, true);
    StartSleeping(head);
    m_connection->Teleport(glm::dvec3(head.x + 0.5 - direction->dx * 0.4, head.y + 0.5625, head.z + 0.5 - direction->dz * 0.4), m_yaw, m_pitch);
    m_server->GetPlayerList()->SendToLevel(m_level, std::shared_ptr<Packet>(new PlayerSleepS2CPacket(m_playerIndex, true, head)));
}

void ServerPlayer::WakeUp()
{
    if (!m_sleeping)
    {
        return;
    }

    StopSleeping();
    BlockPos standUp(m_bedPos.x, m_bedPos.y + 1, m_bedPos.z);
    BedBlock *bed = dynamic_cast<BedBlock *>(Block::ById(m_level->GetBlockId(m_bedPos)));
    if (bed)
    {
        bed->SetOccupied(m_level, m_bedPos, false);
        bed->FindStandUpPosition(m_level, m_bedPos, &standUp);
    }

    m_connection->Teleport(glm::dvec3(standUp.x + 0.5, standUp.y + 0.1, standUp.z + 0.5), m_yaw, m_pitch);
    m_server->GetPlayerList()->SendToLevel(m_level, std::shared_ptr<Packet>(new PlayerSleepS2CPacket(m_playerIndex, false, m_bedPos)));
}

void ServerPlayer::SendStatusMessage(const std::string &key)
{
    m_connection->Send(std::shared_ptr<Packet>(new ChatS2CPacket(CommandContext::Translate(key).dump(), true)));
}

void ServerPlayer::DismountVehicle()
{
    Entity *vehicle = m_vehicle;
    if (!vehicle)
    {
        return;
    }

    StopRiding();
    m_level->OnPassengersChanged(vehicle);
    m_connection->Teleport(vehicle->GetDismountPosition(this), m_yaw, m_pitch);
}

void ServerPlayer::Attack(LivingEntity *target)
{
    ItemInstance *held = m_inventory.GetSelectedItem();
    int damage         = (int) GetAttributeValue(Attribute::s_attackDamage) + (held->IsEmpty() ? 0 : held->GetItem()->GetAttackDamage());
    bool crit          = m_fallDistance > 0.0f && !IsOnLadder() && !IsInWater();
    if (crit)
    {
        damage += m_level->GetRandom().NextInt(0, damage / 2 + 1);
    }

    std::vector<FractureTrait *> fractures = FractureTraits::GetActive(*held);
    int wear                               = 0;
    for (size_t i = 0; i < fractures.size(); i++)
    {
        damage = fractures[i]->ModifyAttackDamage(this, damage);
        wear += fractures[i]->GetAttackWear();
    }

    // The client swings before it sends the attack, so more than one swing since the last hit means a miss
    bool missed      = m_swingsSinceHit > 1;
    m_swingsSinceHit = 0;
    if (!target->Hurt(eDamageSource_Player, damage, this))
    {
        return;
    }

    CauseFoodExhaustion(FoodData::kExhaustionAttack);

    if (!GameType::HasInfiniteItems(GetGameType()) && !held->IsEmpty() && held->GetItem()->GetHitDamage() > 0)
    {
        held->HurtAndBreak(held->GetItem()->GetHitDamage() + wear, this);

        m_server->GetPlayerList()->SendInventoryTo(this);
    }

    if (crit)
    {
        m_server->GetPlayerList()->SendToLevel(m_level, std::shared_ptr<Packet>(new AnimateCritS2CPacket(target->GetId())));
    }

    const AABB &box        = target->GetAABB();
    FractureTrait::Hit hit = {target, (box.GetMin() + box.GetMax()) * 0.5, damage, missed};
    bool knockbackReplaced = false;
    for (size_t i = 0; i < fractures.size(); i++)
    {
        if (fractures[i]->OnAttack(this, hit))
        {
            knockbackReplaced = true;
        }
    }

    if (knockbackReplaced)
    {
        return;
    }

    glm::dvec3 push;
    double knockback = GetAttributeValue(Attribute::s_attackKnockback) + (IsSprinting() ? 1.0 : 0.0) +
                       (kStickFight && held->GetItem() == Item::ByName("stick") ? 1.0 : 0.0);
    if (knockback > 0.0)
    {
        double yawRad = m_yaw * (std::numbers::pi / 180.0);
        push          = glm::dvec3(cos(yawRad) * 0.5 * knockback, 0.1, sin(yawRad) * 0.5 * knockback);
    }

    target->Knockback(m_position, push);
}

void ServerPlayer::OnSwing()
{
    m_swingsSinceHit++;
}

FractureTrait::State *ServerPlayer::GetFractureState(FractureTrait *trait)
{
    std::unique_ptr<FractureTrait::State> &state = m_fractureStates[trait];
    if (!state)
    {
        state.reset(trait->CreateState());
    }

    return state.get();
}

std::vector<std::pair<FractureTrait *, int>> ServerPlayer::GetArmorFractures()
{
    std::vector<std::pair<FractureTrait *, int>> result;
    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        std::vector<FractureTrait *> fractures = FractureTraits::GetActive(*m_inventory.GetArmor(i));
        for (size_t j = 0; j < fractures.size(); j++)
        {
            size_t k = 0;
            while (k < result.size() && result[k].first != fractures[j])
            {
                k++;
            }

            if (k == result.size())
            {
                result.push_back(std::make_pair(fractures[j], 0));
            }

            result[k].second++;
        }
    }

    return result;
}

void ServerPlayer::Knockback(const glm::dvec3 &source, const glm::dvec3 &push)
{
    double sourceX = source.x - m_position.x;
    double sourceZ = source.z - m_position.z;
    while (sourceX * sourceX + sourceZ * sourceZ < 1.0e-4)
    {
        sourceX = (m_level->GetRandom().NextDouble() - m_level->GetRandom().NextDouble()) * 0.01;
        sourceZ = (m_level->GetRandom().NextDouble() - m_level->GetRandom().NextDouble()) * 0.01;
    }

    m_connection->Send(std::shared_ptr<Packet>(new KnockbackS2CPacket((float) sourceX, (float) sourceZ, push)));
}

void ServerPlayer::SendHealth()
{
    m_lastSentHealth          = m_health;
    m_lastSentFoodLevel       = m_foodData.GetFoodLevel();
    m_lastSentSaturationLevel = m_foodData.GetSaturationLevel();
    m_connection->Send(std::shared_ptr<Packet>(new SetHealthS2CPacket(m_health, m_lastSentFoodLevel, m_lastSentSaturationLevel)));
}

void ServerPlayer::GiveExperience(int amount)
{
    int oldLevel = m_experienceLevel;
    AddExperience(amount);
    if (m_experienceLevel / 5 > oldLevel / 5 && m_lastLevelUpSoundTick < m_totalTickCount - 100)
    {
        m_lastLevelUpSoundTick = m_totalTickCount;
        m_level->PlaySound("random.levelup", m_position, 0.75f, 1.0f);
    }

    SendExperience();
}

void ServerPlayer::PickupExperience(int amount)
{
    bool repaired = false;
    for (int i = -1; i < Inventory::s_armorSize && amount > 0; i++)
    {
        ItemInstance *item                     = i < 0 ? m_inventory.GetSelectedItem() : m_inventory.GetArmor(i);
        std::vector<FractureTrait *> fractures = FractureTraits::GetActive(*item);
        for (size_t j = 0; j < fractures.size() && amount > 0; j++)
        {
            int left = fractures[j]->OnPickupExperience(this, item, amount);
            repaired |= left != amount;
            amount = left;
        }
    }

    if (repaired)
    {
        GetServer()->GetPlayerList()->SendInventoryTo(this);
    }

    if (amount > 0)
    {
        GiveExperience(amount);
    }
}

void ServerPlayer::GiveExperienceLevels(int levels)
{
    m_experienceLevel += levels;
    if (m_experienceLevel < 0)
    {
        SetExperience(0.0f, 0, 0);
    }

    if (levels > 0 && m_experienceLevel % 5 == 0 && m_lastLevelUpSoundTick < m_totalTickCount - 100)
    {
        m_lastLevelUpSoundTick = m_totalTickCount;
        m_level->PlaySound("random.levelup", m_position, 0.75f, 1.0f);
    }

    SendExperience();
}

void ServerPlayer::SendExperience()
{
    m_connection->Send(std::shared_ptr<Packet>(new SetExperienceS2CPacket(m_experienceProgress, m_experienceLevel, m_experienceTotal)));
}

int ServerPlayer::GetExperienceCooldown() const
{
    return m_experienceCooldown;
}

void ServerPlayer::SetExperienceCooldown(int ticks)
{
    m_experienceCooldown = ticks;
}

void ServerPlayer::CauseFoodExhaustion(float amount)
{
    if (!GameType::CanHurtPlayer(GetGameType()) || m_flying)
    {
        return;
    }

    m_foodData.AddExhaustion(amount);
}

void ServerPlayer::CauseDamageExhaustion(eDAMAGE_SOURCE source)
{
    if (IsArmorDamage(source))
    {
        CauseFoodExhaustion(FoodData::kExhaustionAttack);
    }
}

void ServerPlayer::CompleteUsingItem()
{
    Random &random = m_level->GetRandom();

    SpawnEatParticles(m_useItem, 16);
    if (m_useItem.GetItem()->GetUseAnimation() == eUseAnim_Drink)
    {
        if (!GameType::HasInfiniteItems(GetGameType()))
        {
            *m_inventory.GetSelectedItem() = ItemInstance(Item::ByName("bucket"));
            m_server->GetPlayerList()->SendInventoryTo(this);
        }

        StopUsingItem();
        return;
    }

    FoodItem *food = (FoodItem *) m_useItem.GetItem();
    if (!GameType::HasInfiniteItems(GetGameType()))
    {
        ItemInstance *held = m_inventory.GetSelectedItem();
        held->ConsumeOne();
        if (held->IsEmpty() && food->GetRemainder())
        {
            *held = ItemInstance(food->GetRemainder());
        }

        m_server->GetPlayerList()->SendInventoryTo(this);
    }

    m_foodData.Eat(food->GetNutrition(), food->GetSaturationModifier());
    // We apply what poison and hunger effects would do over their whole duration at once until there is an effect system
    if (random.NextFloat() < food->GetPoisonChance() && GetHealth() > 1)
    {
        Hurt(eDamageSource_Generic, std::min(food->GetPoisonTicks() / 25, GetHealth() - 1), nullptr);
    }

    if (random.NextFloat() < food->GetHungerChance())
    {
        m_foodData.AddExhaustion(0.025f * (float) food->GetHungerTicks());
    }
    m_level->PlaySound("random.burp", m_position, 0.5f, random.NextFloat() * 0.1f + 0.9f);
    StopUsingItem();
}

void ServerPlayer::StopUsingItem()
{
    // The client applies food when its own eat timer ends, so we resend hunger to undo that if ours gets cancelled first
    bool eating = IsUsingItem() && m_useItem.GetItem()->GetUseAnimation() == eUseAnim_Eat;
    Player::StopUsingItem();
    if (eating)
    {
        SendHealth();
    }
}

// We only travel when walking into a tear, so landing next to one on the other side doesn't send us straight back
void ServerPlayer::TickTear()
{
    BlockPos core;
    FractureCoreBlock *fractureCore = (FractureCoreBlock *) Block::ByName("fracture_core");
    int stability                   = fractureCore->GetTearStability(m_level, m_position, core);
    bool inTear                     = stability >= 0 && m_health > 0;
    if (inTear && !m_inTear && stability == 0)
    {
        Hurt(eDamageSource_Rift, 4, nullptr);
        fractureCore->Collapse(m_level, core);
    }
    else if (inTear && !m_inTear)
    {
        if (stability == 1)
        {
            Hurt(eDamageSource_Rift, 5, nullptr);
            for (int i = 0; i < Inventory::s_slotCount; i++)
            {
                ItemInstance *item = m_inventory.GetSlot(i);
                if (!item->IsEmpty() && item->GetItem()->IsDamageable() && m_level->GetRandom().NextInt(0, 1) == 0)
                {
                    item->HurtAndBreak((int) (item->GetItem()->GetMaxDamage() * (0.1f + m_level->GetRandom().NextFloat() * 0.2f)), this);
                }
            }

            int lost = m_level->GetRandom().NextInt(0, Inventory::s_slotCount - 1);
            if (m_level->GetRandom().NextInt(0, 32) == 0 && !m_inventory.GetSlot(lost)->IsEmpty())
            {
                m_inventory.SetSlot(lost, ItemInstance());
            }

            m_server->GetPlayerList()->SendInventoryTo(this);
        }
        else if (stability == 2)
        {
            Hurt(eDamageSource_Rift, 2, nullptr);
        }

        fractureCore->OnPass(m_level, core, m_level->GetRandom());
        eDIMENSION_ID dimension = m_level->GetDimensionId() == eDimensionId_Overworld ? eDimensionId_Caves : eDimensionId_Overworld;
        ServerLevel *level      = m_server->GetOrCreateLevel(dimension);
        glm::dvec3 spawn        = level->FindPlayerSpawn(BlockPos((int) floor(m_position.x), level->GetWorldSpawn().y, (int) floor(m_position.z)));
        m_server->GetPlayerList()->ChangeDimension(this, dimension, spawn, (float) stability / 3.0f);
        m_fallDistance = 0.0f;
    }

    m_inTear = inTear;
}

void ServerPlayer::TickFractures()
{
    for (auto &entry : m_fractureStates)
    {
        entry.first->Tick(this, entry.second.get());
    }
}

void ServerPlayer::TickSteppedBlocks()
{
    const AABB &aabb = GetAABB();
    int y            = (int) floor(aabb.GetMin().y + 0.001);
    if (y < 0 || y >= Chunk::s_sizeY)
    {
        return;
    }

    for (int x = (int) floor(aabb.GetMin().x); x <= (int) floor(aabb.GetMax().x); x++)
    {
        for (int z = (int) floor(aabb.GetMin().z); z <= (int) floor(aabb.GetMax().z); z++)
        {
            BlockPos pos(x, y, z);
            PressurePlateBlock *plate = dynamic_cast<PressurePlateBlock *>(Block::ById(m_level->GetBlockId(pos)));
            if (plate)
            {
                plate->OnEntityInside(m_level, pos);
            }

            WeightedPressurePlateBlock *weightedPlate = dynamic_cast<WeightedPressurePlateBlock *>(Block::ById(m_level->GetBlockId(pos)));
            if (weightedPlate)
            {
                weightedPlate->OnEntityInside(m_level, pos);
            }

            BlockPos below(x, y - 1, z);
            BrittleIceBlock *ice = dynamic_cast<BrittleIceBlock *>(Block::ById(m_level->GetBlockId(below)));
            if (ice && IsOnGround())
            {
                ice->OnSteppedOn(m_level, below);
            }
        }
    }
}

void ServerPlayer::SetPlayerConnection(PlayerConnection *connection)
{
    delete m_connection;
    m_connection = connection;
}

PlayerConnection *ServerPlayer::GetPlayerConnection() const
{
    return m_connection;
}

EversteadServer *ServerPlayer::GetServer() const
{
    return m_server;
}

ServerPlayerGameMode *ServerPlayer::GetGameMode() const
{
    return m_gameMode;
}

ServerContainerMenu *ServerPlayer::GetContainerMenu() const
{
    return m_containerMenu;
}

void ServerPlayer::SetPid(uint32_t pid)
{
    m_pid = pid;
}

uint32_t ServerPlayer::GetPid() const
{
    return m_pid;
}

void ServerPlayer::SetOperator(bool value)
{
    SetSharedFlag(eEntityFlag_Operator, value);
}

bool ServerPlayer::IsOperator() const
{
    return GetSharedFlag(eEntityFlag_Operator);
}

void ServerPlayer::TickFire()
{
    if (GameType::IsSpectator(GetGameType()))
    {
        m_fireTicks = 0;
    }
    else
    {
        LivingEntity::TickFire();
    }

    bool onFire = m_fireTicks > 0;
    if (onFire != GetSharedFlag(eEntityFlag_OnFire))
    {
        SetSharedFlag(eEntityFlag_OnFire, onFire);
        m_server->GetPlayerList()->BroadcastPlayerFlags(this);
    }
}
