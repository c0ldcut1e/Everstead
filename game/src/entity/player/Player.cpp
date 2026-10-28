#include "Player.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <functional>

#include "../../rendering/Icon.h"
#include "../../utils/Random.h"
#include "../../utils/StringUtils.h"
#include "../../utils/math/Mth.h"
#include "../../world/block/Block.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/item/ArmorItem.h"
#include "../../world/item/BowItem.h"
#include "../../world/item/Item.h"
#include "../../world/particle/ParticleSpawnParams.h"

Player::Player(Level *level, const std::wstring &name) : LivingEntity(level)
{
    m_wasInWater                 = false;
    m_waterChecked               = false;
    m_moveForwardInput           = 0.0f;
    m_moveStrafeInput            = 0.0f;
    m_flyVerticalInput           = 0.0f;
    m_tilt                       = m_yaw;
    m_tiltOld                    = m_tilt;
    m_bob                        = m_pitch;
    m_bobOld                     = m_bob;
    m_fallTilt                   = 0.0f;
    m_fallTiltOld                = 0.0f;
    m_walkBob                    = 0.0f;
    m_walkBobOld                 = 0.0f;
    m_fovModifier                = 1.0f;
    m_fovModifierOld             = 1.0f;
    m_useItemDuration            = 0;
    m_experienceLevel            = 0;
    m_experienceProgress         = 0.0f;
    m_experienceTotal            = 0;
    m_ySlideOffset               = 0.0f;
    m_ySlideOffsetOld            = 0.0f;
    m_playerIndex                = 0;
    m_skinTexturePath            = "";
    m_skinModel                  = ePlayerSkinModel_Classic;
    m_capeTexturePath            = "";
    m_capePosition               = m_position;
    m_capeOldPosition            = m_capePosition;
    m_openContainer              = nullptr;
    m_craftingContainer          = new CraftingContainer(3, 3);
    m_inventoryCraftingContainer = new CraftingContainer(2, 2);
    m_anvilContainer             = new AnvilContainer(this);
    m_gameType                   = eGameType_Survival;
    m_respawnDimension           = eDimensionId_Overworld;
    m_hasRespawnPosition         = false;
    m_respawnForced              = false;
    m_sleeping                   = false;
    m_sleepTimer                 = 0;
    m_bedPos                     = BlockPos(0, 0, 0);
    SetName(name);
}

Player::~Player()
{
    delete m_craftingContainer;
    delete m_inventoryCraftingContainer;
    delete m_anvilContainer;
}

eENTITY_TYPE Player::GetType()
{
    return s_type;
}

Inventory *Player::GetInventory()
{
    return &m_inventory;
}

AbstractContainer *Player::GetOpenContainer() const
{
    return m_openContainer;
}

CraftingContainer *Player::GetCraftingContainer() const
{
    return m_craftingContainer;
}

CraftingContainer *Player::GetInventoryCraftingContainer() const
{
    return m_inventoryCraftingContainer;
}

AnvilContainer *Player::GetAnvilContainer() const
{
    return m_anvilContainer;
}

void Player::Tick()
{
    TickSleeping();
    if (m_sleeping)
    {
        m_velocity = glm::dvec3();
    }
    else if (m_vehicle)
    {
        m_velocity = glm::dvec3();
        m_vehicle->PositionRider(this);
    }
    else if (m_health > 0)
    {
        float inputScale = IsUsingItem() ? 0.2f : 1.0f;
        SetMoveInput(m_moveStrafeInput * inputScale, m_moveForwardInput * inputScale);
        Entity::Tick();
        PushEntities();
    }
    else
    {
        m_velocity = glm::dvec3();
    }

    TickHurt();
    TickCrit();
    UpdateCapePosition();
    UpdateSwing();
}

int Player::GetUpdateInterval() const
{
    return 2;
}

void Player::OpenContainer(AbstractContainer *container)
{
    m_openContainer = container;
}

void Player::CloseContainer()
{
    m_openContainer = nullptr;
}

void Player::StartUsingItem(const ItemInstance &item, int duration)
{
    m_useItem         = item;
    m_useItemDuration = duration;
}

void Player::StopUsingItem()
{
    m_useItem         = ItemInstance();
    m_useItemDuration = 0;
}

bool Player::IsUsingItem() const
{
    return !m_useItem.IsEmpty();
}

bool Player::IsBlocking() const
{
    return IsUsingItem() && m_useItem.GetItem()->GetUseAnimation() == eUseAnim_Block;
}

bool Player::IsDrawingBow() const
{
    return IsUsingItem() && m_useItem.GetItem()->GetUseAnimation() == eUseAnim_Bow;
}

int Player::GetUseItemDuration() const
{
    return m_useItemDuration;
}

int Player::GetTicksUsingItem() const
{
    return m_useItem.GetItem()->GetUseDuration() - m_useItemDuration;
}

const Icon &Player::GetHeldItemIcon()
{
    if (IsDrawingBow())
    {
        return ((BowItem *) m_useItem.GetItem())->GetPullingIcon(GetTicksUsingItem());
    }

    return m_inventory.GetSelectedItem()->GetIcon();
}

void Player::TickUsingItem()
{
    if (!IsUsingItem() || m_useItemDuration <= 0)
    {
        return;
    }

    if (m_inventory.GetSelectedItem()->GetItem() != m_useItem.GetItem())
    {
        StopUsingItem();
        return;
    }

    eUSE_ANIM anim = m_useItem.GetItem()->GetUseAnimation();
    bool eating    = anim == eUseAnim_Eat || anim == eUseAnim_Drink;
    if (eating && m_useItemDuration <= 25 && m_useItemDuration % 4 == 0)
    {
        SpawnEatParticles(m_useItem, 5);
    }

    m_useItemDuration--;
    if (m_useItemDuration == 0)
    {
        if (eating)
        {
            CompleteUsingItem();
        }
        else
        {
            StopUsingItem();
        }
    }
}

void Player::CompleteUsingItem()
{
    SpawnEatParticles(m_useItem, 16);
}

void Player::SpawnEatParticles(const ItemInstance &item, int count)
{
    Random &random = m_level->GetRandom();
    if (item.GetItem()->GetUseAnimation() == eUseAnim_Drink)
    {
        if (!m_level->IsClientSide())
        {
            m_level->PlaySound("random.drink", m_position, 0.5f, random.NextFloat() * 0.1f + 0.9f);
        }

        return;
    }

    if (!m_level->IsClientSide())
    {
        m_level->PlaySound("random.eat" + std::to_string(random.NextInt(1, 3)), m_position, 0.5f + 0.5f * (float) random.NextInt(0, 1),
                           (random.NextFloat() - random.NextFloat()) * 0.2f + 1.0f);
        return;
    }

    const Icon &icon = item.GetIcon();
    glm::dvec3 eye   = m_position + glm::dvec3(0.0, GetEyeHeight(), 0.0);
    for (int i = 0; i < count; i++)
    {
        double vx = (random.NextFloat() - 0.5) * 0.1;
        double vy = random.NextFloat() * 0.1 + 0.1;
        double px = (random.NextFloat() - 0.5) * 0.3;
        double py = -random.NextFloat() * 0.6 - 0.3;

        ParticleSpawnParams params;
        params.type        = eParticleType_ItemCrack;
        params.position    = eye + m_right * px + m_up * py + m_front * 0.6;
        params.velocity    = m_right * vx + m_up * vy + glm::dvec3(0.0, 0.05, 0.0);
        params.hasVelocity = true;
        params.region      = {icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()};
        params.hasRegion   = true;
        m_level->SpawnParticle(params);
    }
}

void Player::SetMoveInputs(float forward, float strafe)
{
    m_moveForwardInput = forward;
    m_moveStrafeInput  = strafe;
}

float Player::GetMoveForwardInput() const
{
    return m_moveForwardInput;
}

float Player::GetMoveStrafeInput() const
{
    return m_moveStrafeInput;
}

void Player::SetFlyVerticalInput(float value)
{
    m_flyVerticalInput = value;
}

void Player::SetPlayerIndex(uint8_t playerIndex)
{
    m_playerIndex = playerIndex;
}

uint8_t Player::GetPlayerIndex() const
{
    return m_playerIndex;
}

void Player::SetName(const std::wstring &name)
{
    Entity::SetName(name);
    SetSkin("assets/texture/models/player/steve.png", ePlayerSkinModel_Classic);
}

std::string Player::GetScoreboardName() const
{
    return StringUtils::ToUtf8(GetName());
}

void Player::SetSkin(const std::string &texturePath, ePLAYER_SKIN_MODEL skinModel)
{
    m_skinTexturePath = texturePath;
    m_skinModel       = skinModel;
}

const std::string &Player::GetSkinTexturePath() const
{
    return m_skinTexturePath;
}

ePLAYER_SKIN_MODEL Player::GetSkinModel() const
{
    return m_skinModel;
}

void Player::SetCape(const std::string &texturePath)
{
    m_capeTexturePath = texturePath;
}

const std::string &Player::GetCapeTexturePath() const
{
    return m_capeTexturePath;
}

glm::dvec3 Player::GetCapePosition(float partialTicks) const
{
    return glm::dvec3(m_capeOldPosition.x + (m_capePosition.x - m_capeOldPosition.x) * partialTicks,
                      m_capeOldPosition.y + (m_capePosition.y - m_capeOldPosition.y) * partialTicks,
                      m_capeOldPosition.z + (m_capePosition.z - m_capeOldPosition.z) * partialTicks);
}

void Player::SetFlying(bool value)
{
    m_flying = value;
}

bool Player::IsFlying() const
{
    return m_flying;
}

void Player::SetGameType(eGAME_TYPE gameType)
{
    m_gameType = gameType;
    SetSharedFlag(eEntityFlag_Spectator, GameType::IsSpectator(gameType));
    if (!GameType::CanFly(gameType) && IsFlying())
    {
        SetFlying(false);
    }

    if (!GameType::IsSpectator(gameType))
    {
        m_attributes.Get(Attribute::s_flyingSpeed)->SetBaseValue(Attribute::s_flyingSpeed.GetDefaultValue());
        SetNoCollision(false);
    }
}

eGAME_TYPE Player::GetGameType() const
{
    return m_gameType;
}

void Player::SetRespawnPosition(const BlockPos &pos, eDIMENSION_ID dimension, bool forced)
{
    m_respawnPosition    = pos;
    m_respawnDimension   = dimension;
    m_hasRespawnPosition = true;
    m_respawnForced      = forced;
}

void Player::ClearRespawnPosition()
{
    m_hasRespawnPosition = false;
    m_respawnForced      = false;
}

bool Player::HasRespawnPosition() const
{
    return m_hasRespawnPosition;
}

bool Player::IsRespawnForced() const
{
    return m_respawnForced;
}

const BlockPos &Player::GetRespawnPosition() const
{
    return m_respawnPosition;
}

eDIMENSION_ID Player::GetRespawnDimension() const
{
    return m_respawnDimension;
}

void Player::StartSleeping(const BlockPos &bedPos)
{
    m_sleeping   = true;
    m_sleepTimer = 0;
    m_bedPos     = bedPos;
    m_velocity   = glm::dvec3();
    StopUsingItem();
}

void Player::StopSleeping()
{
    m_sleeping   = false;
    m_sleepTimer = 0;
}

bool Player::IsSleeping() const
{
    return m_sleeping;
}

int Player::GetSleepTimer() const
{
    return m_sleepTimer;
}

const BlockPos &Player::GetBedPos() const
{
    return m_bedPos;
}

void Player::TickSleeping()
{
    if (m_sleeping && m_sleepTimer < 100)
    {
        m_sleepTimer++;
    }
}

double Player::GetBlockReach() const
{
    return GetAttributeValue(Attribute::s_blockInteractionRange) + (m_gameType == eGameType_Creative ? 0.5 : 0.0);
}

float Player::GetDestroyProgress(Block *block)
{
    float destroyTime = block->GetDestroyTime();
    if (destroyTime < 0.0f)
    {
        return 0.0f;
    }

    ItemInstance *held = m_inventory.GetSelectedItem();
    float speed        = held->IsEmpty() ? 1.0f : held->GetItem()->GetDestroySpeed(block);
    speed *= (float) GetAttributeValue(Attribute::s_blockBreakSpeed);
    if (m_level->IsLiquidAt(m_position + glm::dvec3(0.0, GetEyeHeight(), 0.0), false))
    {
        speed /= 5.0f;
    }

    if (!m_onGround)
    {
        speed /= 5.0f;
    }

    return speed / destroyTime / (CanHarvest(block) ? 30.0f : 100.0f);
}

bool Player::CanHarvest(Block *block)
{
    if (!block->RequiresTool())
    {
        return true;
    }

    ItemInstance *held = m_inventory.GetSelectedItem();
    return !held->IsEmpty() && held->GetItem()->CanHarvest(block);
}

double Player::GetEntityReach() const
{
    return GetAttributeValue(Attribute::s_entityInteractionRange) + (m_gameType == eGameType_Creative ? 3.0 : 0.0);
}

double Player::GetEyeHeight() const
{
    return 1.62;
}

double Player::GetStepHeight() const
{
    return GetAttributeValue(Attribute::s_stepHeight);
}

FoodData *Player::GetFoodData()
{
    return &m_foodData;
}

int Player::GetArmorValue() const
{
    int armor = LivingEntity::GetArmorValue();
    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        const ItemInstance *item = m_inventory.GetArmor(i);
        if (!item->IsEmpty() && item->GetItem()->GetArmorSlot() >= 0)
        {
            armor += ((ArmorItem *) item->GetItem())->GetDefense();
        }
    }

    return std::min(armor, 20);
}

int Player::GetExperienceLevel() const
{
    return m_experienceLevel;
}

float Player::GetExperienceProgress() const
{
    return m_experienceProgress;
}

int Player::GetTotalExperience() const
{
    return m_experienceTotal;
}

int Player::GetXpNeededForNextLevel() const
{
    if (m_experienceLevel >= 30)
    {
        return 62 + (m_experienceLevel - 30) * 7;
    }

    if (m_experienceLevel >= 15)
    {
        return 17 + (m_experienceLevel - 15) * 3;
    }

    return 17;
}

void Player::SetExperience(float progress, int level, int total)
{
    m_experienceProgress = progress;
    m_experienceLevel    = level;
    m_experienceTotal    = total;
}

void Player::AddExperience(int amount)
{
    amount = std::min(amount, INT_MAX - m_experienceTotal);
    m_experienceProgress += (float) amount / (float) GetXpNeededForNextLevel();
    m_experienceTotal = std::max(m_experienceTotal + amount, 0);
    while (m_experienceProgress < 0.0f)
    {
        float points = m_experienceProgress * (float) GetXpNeededForNextLevel();
        if (m_experienceLevel > 0)
        {
            m_experienceLevel--;
            m_experienceProgress = 1.0f + points / (float) GetXpNeededForNextLevel();
        }
        else
        {
            SetExperience(0.0f, 0, 0);
        }
    }

    while (m_experienceProgress >= 1.0f)
    {
        m_experienceProgress = (m_experienceProgress - 1.0f) * (float) GetXpNeededForNextLevel();
        m_experienceLevel++;
        m_experienceProgress /= (float) GetXpNeededForNextLevel();
    }
}

float Player::GetTilt() const
{
    return m_tilt;
}

float Player::GetTiltOld() const
{
    return m_tiltOld;
}

float Player::GetBob() const
{
    return m_bob;
}

float Player::GetBobOld() const
{
    return m_bobOld;
}

float Player::GetFallTilt() const
{
    return m_fallTilt;
}

float Player::GetFallTiltOld() const
{
    return m_fallTiltOld;
}

float Player::GetWalkBob() const
{
    return m_walkBob;
}

float Player::GetWalkBobOld() const
{
    return m_walkBobOld;
}

float Player::GetFovModifier(float partialTicks) const
{
    return m_fovModifierOld + (m_fovModifier - m_fovModifierOld) * partialTicks;
}

void Player::UpdateCapePosition()
{
    m_capeOldPosition = m_capePosition;

    glm::dvec3 delta          = m_position - m_capePosition;
    const double maxCapeDelta = 10.0;
    if (delta.x > maxCapeDelta || delta.x < -maxCapeDelta)
    {
        m_capePosition.x    = m_position.x;
        m_capeOldPosition.x = m_position.x;
        delta.x             = 0.0;
    }

    if (delta.y > maxCapeDelta || delta.y < -maxCapeDelta)
    {
        m_capePosition.y    = m_position.y;
        m_capeOldPosition.y = m_position.y;
        delta.y             = 0.0;
    }

    if (delta.z > maxCapeDelta || delta.z < -maxCapeDelta)
    {
        m_capePosition.z    = m_position.z;
        m_capeOldPosition.z = m_position.z;
        delta.z             = 0.0;
    }

    m_capePosition.x += delta.x * 0.25;
    m_capePosition.y += delta.y * 0.25;
    m_capePosition.z += delta.z * 0.25;
}

void Player::SpawnSprintParticle(const glm::dvec3 &motion)
{
    int blockX   = (int) floor(m_position.x);
    int blockY   = (int) floor(m_position.y - 0.2);
    int blockZ   = (int) floor(m_position.z);
    BlockPos pos = BlockPos(blockX, blockY, blockZ);
    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (!block || block == Block::ById(0))
    {
        return;
    }

    uint32_t color     = 0xFF999999;
    ChunkPos chunkPos  = ChunkPos(Mth::FloorDiv(blockX, Chunk::s_sizeX), Mth::FloorDiv(blockZ, Chunk::s_sizeZ));
    const Chunk *chunk = m_level->GetChunk(chunkPos);
    if (chunk)
    {
        color = MultiplyTint(color, block->ResolveTint(Direction::up, m_level, chunk, Mth::FloorMod(blockX, Chunk::s_sizeX),
                                                       Mth::FloorMod(blockZ, Chunk::s_sizeZ)));
    }

    Block::UVRect rect = block->GetAtlasUVRect(Direction::up);
    ParticleSpawnParams params;
    params.type        = eParticleType_Terrain;
    params.position    = glm::dvec3(m_position.x, m_position.y + 0.1, m_position.z);
    params.velocity    = glm::dvec3(-motion.x * 4.0, 1.5, -motion.z * 4.0);
    params.region      = {rect.u0, rect.v0, rect.u1, rect.v1};
    params.color       = color;
    params.amount      = 1;
    params.hasVelocity = true;
    params.hasRegion   = true;
    params.hasColor    = true;
    m_level->SpawnParticle(params);
}

void Player::UpdateWaterSplash(const glm::dvec3 &motion)
{
    const float playerWidth = 0.6f;

    bool inWater   = IsInWater();
    bool entered   = inWater && !m_wasInWater && m_waterChecked;
    m_wasInWater   = inWater;
    m_waterChecked = true;
    if (!entered)
    {
        return;
    }

    Random &random = m_level->GetRandom();
    float speed    = std::min(1.0f, (float) sqrt(motion.x * motion.x * 0.2 + motion.y * motion.y + motion.z * motion.z * 0.2) * 0.2f);
    m_level->PlaySound("random.splash", m_position, speed, 1.0f + (random.NextFloat() - random.NextFloat()) * 0.4f);

    double surfaceY = floor(m_aabb.GetMin().y) + 1.0;
    int particles   = 1 + (int) (playerWidth * 20.0f);
    for (int i = 0; i < particles; i++)
    {
        ParticleSpawnParams params;
        params.type        = eParticleType_Splash;
        params.position    = glm::dvec3(m_position.x + (random.NextFloat() * 2.0f - 1.0f) * playerWidth, surfaceY,
                                        m_position.z + (random.NextFloat() * 2.0f - 1.0f) * playerWidth);
        params.velocity    = glm::dvec3(motion.x, motion.y, motion.z);
        params.hasVelocity = true;
        m_level->SpawnParticle(params);
    }
}
