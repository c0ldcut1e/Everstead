#ifndef EVERSTEAD_DEDICATED_SERVER

#include "LocalPlayer.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "../../core/Everstead.h"
#include "../../network/listener/ClientConnection.h"
#include "../../network/packet/c2s/AttackC2SPacket.h"
#include "../../network/packet/c2s/ContainerClickC2SPacket.h"
#include "../../network/packet/c2s/InteractC2SPacket.h"
#include "../../network/packet/c2s/PickBlockC2SPacket.h"
#include "../../ui/Gui.h"
#include "../../ui/control/UIController.h"
#include "../../ui/scene/UISceneId.h"
#include "../../utils/Random.h"
#include "../../utils/Time.h"
#include "../../utils/math/Mth.h"
#include "../../world/ItemInHandRenderer.h"
#include "../../world/LevelRenderer.h"
#include "../../world/block/BedBlock.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRegistry.h"
#include "../../world/block/CropBlock.h"
#include "../../world/block/EchoGlassBlock.h"
#include "../../world/block/FenceBlock.h"
#include "../../world/block/FractureCoreBlock.h"
#include "../../world/block/LilyPadBlock.h"
#include "../../world/block/PointedDripstoneBlock.h"
#include "../../world/block/ScaffoldingBlock.h"
#include "../../world/block/SignBlock.h"
#include "../../world/block/StemBlock.h"
#include "../../world/block/WallBlock.h"
#include "../../world/blockentity/PistonMovingBlockEntity.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/crafting/FurnaceRecipes.h"
#include "../../world/crafting/Recipes.h"
#include "../../world/inventory/InventoryMenu.h"
#include "../../world/inventory/ResonatorContainer.h"
#include "../../world/item/BoatItem.h"
#include "../../world/item/BucketItem.h"
#include "../../world/item/CreativeTabs.h"
#include "../../world/item/FlintAndSteelItem.h"
#include "../../world/item/FoodItem.h"
#include "../../world/item/Item.h"
#include "../../world/item/SpawnEggItem.h"
#include "../../world/item/SwordItem.h"
#include "../../world/item/ThrowableItem.h"
#include "../../world/loot/LootTables.h"
#include "../Mob.h"
#include "../item/Boat.h"
#include "../item/LeashKnot.h"
#include "../item/Minecart.h"
#include "MultiPlayerLocalPlayer.h"

const float LocalPlayer::s_sprintForwardThreshold = 0.8f;
const float LocalPlayer::s_sprintFovModifier      = 1.15f;
const float LocalPlayer::s_flyFovModifier         = 1.1f;

LocalPlayer::LocalPlayer(Level *level, const std::wstring &name) : Player(level, name)
{
    m_renderPlayer = new RemotePlayer(level, name);
    m_renderPlayer->SetSilent(true);
    m_thirdPersonView     = eThirdPersonView_FirstPerson;
    m_sprintKeyDown       = false;
    m_forwardTapsThisTick = 0;
    m_sprintTriggerTime   = 0;
    m_jumpPressCount      = 0;
    m_jumpTriggerTime     = 0;
    m_attackHeld          = false;
    m_useHeld             = false;
    m_attackRepeatTime    = 0;
    m_useRepeatTime       = 0;
    m_destroyHeld         = false;
    m_isDestroying        = false;
    m_destroyPos          = BlockPos(0, 0, 0);
    m_destroyProgress     = 0.0f;
    m_destroyTicks        = 0;
    m_destroyDelay        = 0;
    m_destroyStage        = -1;
    m_caveMood            = 0.0f;
    m_inTear              = false;
    m_caveSoundsPlayed    = 0;
    m_recipeView          = new CraftingContainer(3, 3);
    m_recipeViewParent    = nullptr;
    m_recipeViewPage      = 0;
    m_recipeViewStartMs   = 0;
    m_chestContainer      = nullptr;
    m_chestMenuId         = InventoryMenu::s_chestMenuId;
}

LocalPlayer::~LocalPlayer()
{
    delete m_renderPlayer;
    m_renderPlayer = nullptr;
    delete m_recipeView;
    delete m_chestContainer;
}

eENTITY_TYPE LocalPlayer::GetType()
{
    return s_type;
}

void LocalPlayer::Update(float partialTicks)
{
    float yaw = m_yaw;
    if (dynamic_cast<Boat *>(m_vehicle))
    {
        yaw += m_vehicle->GetRenderYaw(partialTicks) - m_vehicle->GetYaw();
    }

    m_renderPlayer->SetInstantHeadRotation(yaw, m_pitch);
}

glm::dvec3 LocalPlayer::GetEyePosition(float partialTicks) const
{
    float ySlide = m_ySlideOffsetOld + (m_ySlideOffset - m_ySlideOffsetOld) * partialTicks;
    return GetRenderPosition(partialTicks) + glm::dvec3(0.0, GetEyeHeight() - ySlide, 0.0);
}

bool LocalPlayer::IsEyeInWater() const
{
    return m_level->IsLiquidAt(GetEyePosition(1.0f));
}

RemotePlayer *LocalPlayer::GetRenderPlayer() const
{
    return m_renderPlayer;
}

void LocalPlayer::Tick()
{
    Everstead::GetInstance()->GetAccountService()->ApplyPlayerLook(this);

    m_tiltOld         = m_tilt;
    m_bobOld          = m_bob;
    m_fallTiltOld     = m_fallTilt;
    m_walkBobOld      = m_walkBob;
    m_ySlideOffsetOld = m_ySlideOffset;

    m_tilt += (m_yaw - m_tilt) * 0.5f;
    m_bob += (m_pitch - m_bob) * 0.5f;

    bool spectator = GameType::IsSpectator(m_gameType);
    if (spectator)
    {
        m_jumpPressCount = 0;
        if (!m_flying)
        {
            SetFlying(true);
        }
    }
    else
    {
        UpdateFlyToggle();
    }

    SetNoCollision(spectator);

    UpdateSprinting();
    UpdateClickRepeat();
    if (!spectator && !m_vehicle)
    {
        PushOutOfBlocks();
    }

    // We start the transition ourselves on entering a tear so it covers the screen before the server moves us
    DimensionTransition *transition = Everstead::GetInstance()->GetGameRenderer()->GetDimensionTransition();
    FractureCoreBlock *fractureCore = (FractureCoreBlock *) Block::ByName("fracture_core");
    BlockPos core;
    int tearStability = fractureCore->GetTearStability(m_level, m_position, core);
    bool inTear       = tearStability >= 0 && m_health > 0 && !spectator;
    if (inTear && !m_inTear && tearStability > 0)
    {
        transition->SetStability(tearStability / 3.0f);
        Everstead::GetInstance()->StartDimensionTransition();
        transition->SetTear(fractureCore->GetTearCenter(core), fractureCore->GetTearSeed(core));
    }

    m_inTear = inTear;

    TickUsingItem();
    glm::dvec3 startPosition = m_position;
    Player::Tick();
    if (!spectator)
    {
        UpdateStepSound(m_position - startPosition, m_onGround);
        UpdateWaterSplash(m_position - startPosition);
        UpdateFall(m_position, m_position.y - startPosition.y, m_onGround);
    }

    TickAirSupply();
    UpdateCaveMood();

    if (m_flying && m_onGround && !spectator)
    {
        SetFlying(false);
    }

    float targetFallTilt = atanf(-(float) m_velocity.y * 0.2f) * 15.0f;
    if (m_onGround)
    {
        targetFallTilt = 0.0f;
    }

    m_fallTilt += (targetFallTilt - m_fallTilt) * 0.8f;

    float targetWalkBob = sqrtf((float) (m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z));
    if (targetWalkBob > 0.1f)
    {
        targetWalkBob = 0.1f;
    }

    if (!m_onGround)
    {
        targetWalkBob = 0.0f;
    }

    m_walkBob += (targetWalkBob - m_walkBob) * 0.4f;

    if (IsSneaking() && m_ySlideOffset < 0.2f)
    {
        m_ySlideOffset = 0.2f;
    }

    m_ySlideOffset *= 0.4f;

    m_fovModifierOld        = m_fovModifier;
    float targetFovModifier = 1.0f;
    if (IsSprinting())
    {
        targetFovModifier = s_sprintFovModifier;
    }
    else if (m_flying)
    {
        targetFovModifier = s_flyFovModifier;
    }

    m_fovModifier += (targetFovModifier - m_fovModifier) * 0.5f;

    if (IsSprinting() && m_onGround)
    {
        SpawnSprintParticle(m_velocity);
    }

    m_renderPlayer->StoreOld();
    m_renderPlayer->SetName(m_name);
    m_renderPlayer->SetPlayerIndex(m_playerIndex);
    m_renderPlayer->SetSkin(m_skinTexturePath, m_skinModel);
    m_renderPlayer->SetCape(m_capeTexturePath);
    m_renderPlayer->SetGrounded(m_onGround);
    m_renderPlayer->SetSneaking(IsSneaking());
    m_renderPlayer->SetRiding(m_vehicle != nullptr);
    m_renderPlayer->SetSharedFlag(eEntityFlag_OnFire, GetSharedFlag(eEntityFlag_OnFire));
    m_renderPlayer->SetSharedFlag(eEntityFlag_Spectator, GameType::IsSpectator(m_gameType));

    Inventory *renderInventory = m_renderPlayer->GetInventory();
    int selectedSlot           = GetInventory()->GetSelectedSlot();
    renderInventory->SetSelectedSlot(selectedSlot);
    if (ItemInstance *selectedItem = GetInventory()->GetSelectedItem())
    {
        renderInventory->SetItem(selectedSlot, *selectedItem);
    }

    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        *renderInventory->GetArmor(i) = *GetInventory()->GetArmor(i);
    }

    if (IsUsingItem())
    {
        m_renderPlayer->StartUsingItem(m_useItem, m_useItemDuration);
    }
    else
    {
        m_renderPlayer->StopUsingItem();
    }

    m_renderPlayer->SetHealth(m_health);
    if (m_sleeping && !m_renderPlayer->IsSleeping())
    {
        m_renderPlayer->StartSleeping(m_bedPos);
    }
    else if (!m_sleeping && m_renderPlayer->IsSleeping())
    {
        m_renderPlayer->StopSleeping();
    }

    m_renderPlayer->ApplyRenderState(m_position, m_yaw, m_pitch);
    m_renderPlayer->Tick();

    Everstead *everstead       = Everstead::GetInstance();
    UIController *uiController = everstead->GetUiController();
    bool deathMenuOpen         = uiController->IsSceneOpen(eUIScene_DeathMenu);
    if (m_health <= 0 && !deathMenuOpen)
    {
        if (everstead->GetGui()->IsChatOpen())
        {
            everstead->GetGui()->CloseChat();
        }

        everstead->ClosePauseMenu();
        everstead->OpenInventory(eUIScene_DeathMenu);
    }
    else if (m_health > 0 && deathMenuOpen)
    {
        everstead->CloseInventory();
    }

    bool sleepMenuOpen = uiController->IsSceneOpen(eUIScene_SleepMenu);
    if (m_sleeping && m_health > 0 && !sleepMenuOpen)
    {
        everstead->ClosePauseMenu();
        everstead->OpenInventory(eUIScene_SleepMenu);
    }
    else if (!m_sleeping && sleepMenuOpen)
    {
        everstead->CloseInventory();
    }
}

void LocalPlayer::HurtTo(int health)
{
    int damage = m_health - health;
    if (damage <= 0)
    {
        SetHealth(health);
        if (damage < 0)
        {
            // Health packets arrive before this tick's decrement, the extra tick keeps the heal flash on screen
            m_invulnerableTime = s_invulnerableDuration / 2 + 1;
        }
        return;
    }

    m_lastHurt   = damage;
    m_lastHealth = m_health;
    AnimateServerHurt();
    SetHealth(health);
}

void LocalPlayer::AnimateServerHurt()
{
    AnimateHurt();
    m_renderPlayer->AnimateHurt();
}

void LocalPlayer::ClearInputs()
{
    SetMoveInputs(0.0f, 0.0f);
    SetJumping(false);
    SetSneaking(false);
    m_sprintKeyDown = false;
    m_attackHeld    = false;
    m_useHeld       = false;
    m_destroyHeld   = false;
}

// A press acts right away, holding repeats every s_clickRepeatTicks after that
void LocalPlayer::SetClickInputs(bool attackPressed, bool attackHeld, bool destroyHeld, bool usePressed, bool useHeld, bool useItemHeld)
{
    m_attackHeld               = attackHeld;
    m_destroyHeld              = destroyHeld;
    ItemInstance *selectedItem = GetInventory()->GetSelectedItem();
    m_useHeld                  = useHeld || (useItemHeld && !selectedItem->IsEmpty() && selectedItem->GetItem()->GetUseAnimation() != eUseAnim_None);
    bool usingItem             = IsUsingItem();
    if (usingItem && !useItemHeld)
    {
        StopUsingItem();
        SendUseItemAction(ePlayerAction_ReleaseUseItem);
    }

    if (attackPressed)
    {
        BreakTargetedBlock(true);
        m_attackRepeatTime = s_clickRepeatTicks;
    }

    if (usePressed && !usingItem)
    {
        PlaceTargetedBlock();
        m_useRepeatTime = s_clickRepeatTicks;
    }
}

void LocalPlayer::CompleteUsingItem()
{
    Player::CompleteUsingItem();
    // We apply the food before the server's health update arrives so a held use can't start eating again at full hunger
    if (m_useItem.GetItem()->GetUseAnimation() == eUseAnim_Eat)
    {
        FoodItem *food = (FoodItem *) m_useItem.GetItem();
        m_foodData.Eat(food->GetNutrition(), food->GetSaturationModifier());
    }

    StopUsingItem();
}

void LocalPlayer::SendUseItemAction(ePLAYER_ACTION action)
{
    MultiPlayerLocalPlayer *player = (MultiPlayerLocalPlayer *) this;
    player->SendCarriedItemIfNeeded();
    player->GetClientConnection()->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(action, BlockPos(0, 0, 0), 0, 0)));
}

void LocalPlayer::UpdateClickRepeat()
{
    if (GameType::HasInfiniteItems(m_gameType))
    {
        if (m_attackHeld && --m_attackRepeatTime <= 0)
        {
            BreakTargetedBlock(false);
            m_attackRepeatTime = s_clickRepeatTicks;
        }
    }
    else
    {
        ContinueDestroy();
    }

    if (m_useRepeatTime > 0)
    {
        m_useRepeatTime--;
    }

    if (m_useHeld && !IsUsingItem() && m_useRepeatTime <= 0)
    {
        PlaceTargetedBlock();
        m_useRepeatTime = s_clickRepeatTicks;
    }
}

void LocalPlayer::SetJumpHeld(bool value)
{
    SetJumping(value);
}

void LocalPlayer::RegisterJumpPress()
{
    m_jumpPressCount++;
}

void LocalPlayer::SetSprintHeld(bool value)
{
    m_sprintKeyDown = value;
}

void LocalPlayer::SetSneakHeld(bool value)
{
    SetSneaking(value);
}

void LocalPlayer::SetForwardTaps(int count)
{
    m_forwardTapsThisTick += count;
}

void LocalPlayer::UpdateSprinting()
{
    if (m_sprintTriggerTime > 0)
    {
        m_sprintTriggerTime--;
    }

    if (IsSneaking())
    {
        m_sprintTriggerTime = 0;
    }

    bool forwardHeld = m_moveForwardInput > s_sprintForwardThreshold;
    bool hasFood     = m_foodData.GetFoodLevel() > 6 || GameType::CanFly(m_gameType);
    bool canStart    = !IsSprinting() && !IsSneaking() && forwardHeld && hasFood && !IsUsingItem();

    if (canStart && m_sprintKeyDown)
    {
        SetSprinting(true);
    }

    for (int i = 0; i < m_forwardTapsThisTick; i++)
    {
        if (m_sprintTriggerTime > 0)
        {
            if (canStart && m_onGround)
            {
                SetSprinting(true);
            }

            m_sprintTriggerTime = 0;
        }
        else
        {
            m_sprintTriggerTime = s_sprintTriggerWindowTicks;
        }
    }

    m_forwardTapsThisTick = 0;

    if (IsSprinting() && (m_moveForwardInput < s_sprintForwardThreshold || !hasFood || IsUsingItem() || (IsSneaking() && !IsFlying())))
    {
        SetSprinting(false);
    }
}

void LocalPlayer::UpdateFlyToggle()
{
    int presses      = m_jumpPressCount;
    m_jumpPressCount = 0;
    for (int i = 0; i < presses; i++)
    {
        if (m_jumpTriggerTime == 0)
        {
            m_jumpTriggerTime = s_flyToggleWindowTicks;
        }
        else
        {
            if (GameType::CanFly(m_gameType))
            {
                SetFlying(!IsFlying());
            }

            m_jumpTriggerTime = 0;
        }
    }

    if (m_jumpTriggerTime > 0)
    {
        m_jumpTriggerTime--;
    }
}

void LocalPlayer::ApplyLookInput(double dx, double dy)
{
    float sensitivity = exp2f((Everstead::GetInstance()->options.mouseSensitivity - 100.0f) * 0.03f) * 0.08f;
    dx *= sensitivity;
    dy *= sensitivity;

    // Yaw is saved with the world and a float this big loses the precision small mouse movements need, so we keep it in one turn
    float yaw  = m_yaw + (float) dx;
    m_yaw      = remainderf(yaw, 360.0f);
    float wrap = m_yaw - yaw;
    // The hand sway trails the yaw, so it has to move by the same turn when the yaw wraps
    m_tilt += wrap;
    m_tiltOld += wrap;
    m_pitch += dy;

    if (m_pitch < -89.0f)
    {
        m_pitch = -89.0f;
    }

    if (m_pitch > 89.0f)
    {
        m_pitch = 89.0f;
    }

    UpdateVectors();
}

void LocalPlayer::ResetRotation(float yaw, float pitch)
{
    SetRotation(yaw, pitch);
    m_tilt    = yaw;
    m_tiltOld = yaw;
    m_bob     = pitch;
    m_bobOld  = pitch;
}

HitResult *LocalPlayer::Pick() const
{
    glm::dvec3 origin    = GetEyePosition(Everstead::GetInstance()->GetRenderPartialTicks());
    glm::dvec3 direction = glm::normalize(m_front);
    HitResult *hit       = m_level->Clip(origin, direction, (float) GetBlockReach());
    if (hit->IsEntity() && hit->GetDistance() > GetEntityReach())
    {
        hit->SetMiss();
    }

    return hit;
}

void LocalPlayer::BreakTargetedBlock(bool canAttack)
{
    if (GameType::IsSpectator(m_gameType))
    {
        return;
    }

    Swing();
    HitResult *hit = Pick();
    if (hit->IsEntity())
    {
        ClientConnection *connection = ((MultiPlayerLocalPlayer *) this)->GetClientConnection();
        Entity *target               = m_level->GetEntityById(hit->GetEntityId());
        if (canAttack && connection &&
            (dynamic_cast<LivingEntity *>(target) || dynamic_cast<Boat *>(target) || dynamic_cast<Minecart *>(target) ||
             dynamic_cast<LeashKnot *>(target)))
        {
            connection->Send(std::shared_ptr<Packet>(new AttackC2SPacket(target->GetId())));
            if (IsSprinting())
            {
                m_velocity.x *= 0.6;
                m_velocity.z *= 0.6;
                SetSprinting(false);
            }
        }
    }
    else if (hit->IsBlock() && GameType::MayBuild(m_gameType) && m_level->GetWorldBorder().IsWithinBounds(hit->GetBlockPos()))
    {
        BlockPos blockPos = hit->GetBlockPos();
        Direction *face   = hit->GetBlockFace();
        BlockPos firePos(blockPos.x + face->dx, blockPos.y + face->dy, blockPos.z + face->dz);
        if (Block::ById(m_level->GetBlockId(firePos)) == Block::ByName("fire"))
        {
            static thread_local Random random;
            glm::dvec3 center((double) firePos.x + 0.5, (double) firePos.y + 0.5, (double) firePos.z + 0.5);
            m_level->PlaySound("random.fizz", center, 0.5f, 2.6f + (random.NextFloat() - random.NextFloat()) * 0.8f);
            DestroyBlock(firePos);
        }
        else if (!GameType::HasInfiniteItems(m_gameType))
        {
            StartDestroy(blockPos);
        }
        else if (!dynamic_cast<SwordItem *>(GetInventory()->GetSelectedItem()->GetItem()))
        {
            SpawnDestroyParticlesAt(blockPos);
            m_level->PlayBlockBreakSound(blockPos, Block::ById(m_level->GetBlockId(blockPos)));
            DestroyBlock(blockPos);
        }
    }

    delete hit;
}

const BlockPos &LocalPlayer::GetDestroyPos() const
{
    return m_destroyPos;
}

int LocalPlayer::GetDestroyStage() const
{
    return m_destroyStage;
}

void LocalPlayer::StartDestroy(const BlockPos &pos)
{
    if (m_isDestroying)
    {
        StopDestroy(true);
    }

    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (GetDestroyProgress(block) >= 1.0f)
    {
        SpawnDestroyParticlesAt(pos);
        m_level->PlayBlockBreakSound(pos, block);
        WearHeldItem(pos, block);
        DestroyBlock(pos);
        return;
    }

    ClientConnection *connection = ((MultiPlayerLocalPlayer *) this)->GetClientConnection();
    if (connection)
    {
        ((MultiPlayerLocalPlayer *) this)->SendCarriedItemIfNeeded();
        connection->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_StartDestroy, pos, 0, 0)));
    }

    m_isDestroying    = true;
    m_destroyPos      = pos;
    m_destroyProgress = 0.0f;
    m_destroyTicks    = 0;
    m_destroyStage    = -1;
}

void LocalPlayer::ContinueDestroy()
{
    if (!m_destroyHeld)
    {
        if (m_isDestroying)
        {
            StopDestroy(true);
        }

        return;
    }

    // A moving piston only lasts a few ticks and can't be targeted, so we keep breaking the block it carries instead of restarting
    if (m_isDestroying && Block::ById(m_level->GetBlockId(m_destroyPos)) == Block::ByName("moving_piston"))
    {
        Swing();
        m_destroyProgress += GetDestroyProgress(((PistonMovingBlockEntity *) m_level->GetBlockEntity(m_destroyPos))->GetBlock());
        return;
    }

    HitResult *hit  = Pick();
    bool hitBlock   = hit->IsBlock() && GameType::MayBuild(m_gameType) && m_level->GetWorldBorder().IsWithinBounds(hit->GetBlockPos());
    BlockPos pos    = hitBlock ? hit->GetBlockPos() : BlockPos(0, 0, 0);
    Direction *face = hit->GetBlockFace();
    delete hit;
    if (!hitBlock)
    {
        if (m_isDestroying)
        {
            StopDestroy(true);
        }

        return;
    }

    SpawnCrackParticle(pos, face);
    if (m_destroyDelay > 0)
    {
        Swing();
        m_destroyDelay--;
        return;
    }

    if (!m_isDestroying || pos != m_destroyPos)
    {
        StartDestroy(pos);
        return;
    }

    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (m_level->GetBlockId(pos) == 0)
    {
        StopDestroy(true);
        return;
    }

    Swing();
    m_destroyProgress += GetDestroyProgress(block);
    if (m_destroyTicks % 4 == 0)
    {
        static thread_local Random random;
        int variants      = 1;
        std::string sound = block->GetStepSound(&variants);
        glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
        m_level->PlaySound(sound + std::to_string(random.NextInt(1, variants)), center, 0.25f, 0.5f);
    }

    m_destroyTicks++;
    if (m_destroyProgress >= 1.0f)
    {
        SpawnDestroyParticlesAt(pos);
        m_level->PlayBlockBreakSound(pos, block);
        WearHeldItem(pos, block);
        DestroyBlock(pos);
        StopDestroy(false);
        m_destroyDelay = 5;
        return;
    }

    m_destroyStage = (int) (m_destroyProgress * 10.0f) - 1;
}

void LocalPlayer::StopDestroy(bool abort)
{
    ClientConnection *connection = ((MultiPlayerLocalPlayer *) this)->GetClientConnection();
    if (abort && connection)
    {
        connection->Send(std::shared_ptr<Packet>(new PlayerActionC2SPacket(ePlayerAction_AbortDestroy, m_destroyPos, 0, 0)));
    }

    m_isDestroying    = false;
    m_destroyProgress = 0.0f;
    m_destroyStage    = -1;
}

void LocalPlayer::Swing()
{
    Player::Swing();
    if (m_renderPlayer)
    {
        m_renderPlayer->Swing();
    }
}

void LocalPlayer::SpawnDestroyParticlesAt(const BlockPos &pos)
{
    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (!block || block == Block::ByName("moving_piston"))
    {
        return;
    }

    const Chunk *blockChunk = nullptr;
    AABB blockAabb(glm::dvec3((double) pos.x, (double) pos.y, (double) pos.z),
                   glm::dvec3((double) pos.x + 1.0, (double) pos.y + 1.0, (double) pos.z + 1.0));

    ChunkPos chunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ));
    const Chunk *chunk = m_level->GetChunk(chunkPos);
    if (chunk)
    {
        blockChunk = chunk;
        int lx     = Mth::FloorMod(pos.x, Chunk::s_sizeX);
        int ly     = Mth::FloorMod(pos.y, Chunk::s_sizeY);
        int lz     = Mth::FloorMod(pos.z, Chunk::s_sizeZ);
        blockAabb  = block->GetPlacedAABB(pos, chunk->GetBlockData(lx, ly, lz));
    }

    SpawnDestroyParticles(pos, blockAabb, block, blockChunk);
}

void LocalPlayer::SpawnCrackParticle(const BlockPos &pos, Direction *face)
{
    static thread_local Random random;

    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (!block || block == Block::ByName("moving_piston"))
    {
        return;
    }

    const Chunk *chunk    = m_level->GetChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)));
    AABB aabb             = block->GetPlacedAABB(pos, m_level->GetBlockData(pos));
    const glm::dvec3 &min = aabb.GetMin();
    const glm::dvec3 &max = aabb.GetMax();
    double inset          = 0.1;
    glm::dvec3 position(min.x + inset + random.NextDouble() * (max.x - min.x - inset * 2.0),
                        min.y + inset + random.NextDouble() * (max.y - min.y - inset * 2.0),
                        min.z + inset + random.NextDouble() * (max.z - min.z - inset * 2.0));
    if (face == Direction::down)
    {
        position.y = min.y - inset;
    }
    else if (face == Direction::up)
    {
        position.y = max.y + inset;
    }
    else if (face == Direction::north)
    {
        position.z = min.z - inset;
    }
    else if (face == Direction::south)
    {
        position.z = max.z + inset;
    }
    else if (face == Direction::west)
    {
        position.x = min.x - inset;
    }
    else if (face == Direction::east)
    {
        position.x = max.x + inset;
    }

    ParticleSpawnParams params = GetTerrainParticleParams(pos, block, chunk);
    params.position            = position;
    params.power               = 0.2;
    params.scale               = 0.6f;
    m_level->SpawnParticle(params);
}

void LocalPlayer::UseBlock(const BlockPos &pos, Direction *face)
{
    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (dynamic_cast<EchoGlassBlock *>(block))
    {
        ((EchoGlassBlock *) block)->ToggleFace(m_level, pos, face);
    }
    else if (block)
    {
        block->OnUse(m_level, pos);
    }
}

void LocalPlayer::UseItemOn(const BlockPos &pos, Direction *face)
{
    GetInventory()->GetSelectedItem()->GetItem()->UseOn(m_level, pos, face, m_front);
}

void LocalPlayer::PlaceTargetedBlock()
{
    HitResult *hit = Pick();
    if (hit->IsBlock() && !m_level->GetWorldBorder().IsWithinBounds(hit->GetBlockPos()))
    {
        delete hit;
        return;
    }

    if (GameType::IsSpectator(m_gameType))
    {
        delete hit;
        return;
    }

    if (hit->IsEntity())
    {
        Entity *entity               = m_level->GetEntityById(hit->GetEntityId());
        Mob *mob                     = dynamic_cast<Mob *>(entity);
        ClientConnection *connection = ((MultiPlayerLocalPlayer *) this)->GetClientConnection();
        if ((dynamic_cast<Boat *>(entity) || dynamic_cast<Minecart *>(entity)) && connection && !IsSneaking())
        {
            connection->Send(std::shared_ptr<Packet>(new InteractC2SPacket(entity->GetId())));
            Swing();
            delete hit;
            return;
        }

        if (dynamic_cast<LeashKnot *>(entity) && connection)
        {
            connection->Send(std::shared_ptr<Packet>(new InteractC2SPacket(entity->GetId())));
            Swing();
            delete hit;
            return;
        }

        if (mob && connection)
        {
            connection->Send(std::shared_ptr<Packet>(new InteractC2SPacket(mob->GetId())));
            if (mob->InteractLeash(this) || mob->Interact(this))
            {
                Swing();
                delete hit;
                return;
            }
        }
    }

    if (hit->IsBlock() && IsSneaking() && dynamic_cast<EchoGlassBlock *>(Block::ById(m_level->GetBlockId(hit->GetBlockPos()))))
    {
        Swing();
        UseBlock(hit->GetBlockPos(), hit->GetBlockFace());
        delete hit;
        return;
    }

    if (hit->IsBlock() && !IsSneaking())
    {
        Block *target      = Block::ById(m_level->GetBlockId(hit->GetBlockPos()));
        ItemInstance *held = GetInventory()->GetSelectedItem();
        bool leashFence    = dynamic_cast<FenceBlock *>(target) && !dynamic_cast<WallBlock *>(target) &&
                             (held->IsEmpty() || held->GetItem() == Item::ByName("lead"));
        if (target && (leashFence || target->IsUsable(m_level->GetBlockData(hit->GetBlockPos()))))
        {
            Swing();
            UseBlock(hit->GetBlockPos(), hit->GetBlockFace());
            delete hit;
            return;
        }
    }

    ItemInstance *selectedItem = GetInventory()->GetSelectedItem();
    eUSE_ANIM anim             = selectedItem->IsEmpty() ? eUseAnim_None : selectedItem->GetItem()->GetUseAnimation();
    Block *block               = selectedItem->IsEmpty() ? nullptr : selectedItem->GetItem()->GetBlock();
    bool canPlace              = block && hit->IsBlock() && GameType::MayBuild(m_gameType);
    if (anim == eUseAnim_Eat && canPlace && PlaceHeldBlock(hit, selectedItem, block))
    {
        delete hit;
        return;
    }

    if (anim == eUseAnim_Block || anim == eUseAnim_Drink ||
        (anim == eUseAnim_Bow && (GameType::HasInfiniteItems(m_gameType) || GetInventory()->FindItem(Item::ByName("arrow")))) ||
        (anim == eUseAnim_Eat && GameType::CanHurtPlayer(m_gameType) && m_foodData.NeedsFood()))
    {
        delete hit;
        StartUsingItem(*selectedItem, selectedItem->GetItem()->GetUseDuration());
        SendUseItemAction(ePlayerAction_StartUsingItem);
        return;
    }

    if (!selectedItem->IsEmpty() && dynamic_cast<ThrowableItem *>(selectedItem->GetItem()))
    {
        delete hit;
        Everstead::GetInstance()->GetLevelRenderer()->GetItemInHandRenderer()->ResetEquip();
        if (!GameType::HasInfiniteItems(m_gameType))
        {
            selectedItem->ConsumeOne();
        }

        SendUseItemAction(ePlayerAction_StartUsingItem);
        return;
    }

    int armorSlot = selectedItem->IsEmpty() ? -1 : selectedItem->GetItem()->GetArmorSlot();
    if (armorSlot >= 0 && GetInventory()->GetArmor(armorSlot)->IsEmpty())
    {
        delete hit;
        *GetInventory()->GetArmor(armorSlot) = *selectedItem;
        *selectedItem                        = ItemInstance();
        Everstead::GetInstance()->GetLevelRenderer()->GetItemInHandRenderer()->ResetEquip();
        SendUseItemAction(ePlayerAction_StartUsingItem);
        return;
    }

    if (!GameType::MayBuild(m_gameType))
    {
        delete hit;
        return;
    }

    BucketItem *bucket = (selectedItem && !selectedItem->IsEmpty()) ? dynamic_cast<BucketItem *>(selectedItem->GetItem()) : nullptr;
    if (bucket)
    {
        delete hit;
        hit = m_level->Clip(GetEyePosition(Everstead::GetInstance()->GetRenderPartialTicks()), glm::normalize(m_front), (float) GetBlockReach(),
                            false, !bucket->GetLiquid());
        if (hit->IsBlock() && m_level->GetWorldBorder().IsWithinBounds(hit->GetBlockPos()) &&
            bucket->CanUse(m_level, hit->GetBlockPos(), hit->GetBlockFace()))
        {
            BlockPos pos    = hit->GetBlockPos();
            Direction *face = hit->GetBlockFace();
            bucket->UpdateHeldItem(this, m_level, pos);
            Everstead::GetInstance()->GetLevelRenderer()->GetItemInHandRenderer()->ResetEquip();
            UseItemOn(pos, face);
            // We place the liquid locally right away because waiting for the server round trip makes water clutches land late
            if (bucket->GetLiquid())
            {
                m_level->SetBlock(BlockPos(pos.x + face->dx, pos.y + face->dy, pos.z + face->dz), bucket->GetLiquid());
            }
            else
            {
                m_level->SetBlock(pos, Block::ByName("air"));
            }
        }

        delete hit;
        return;
    }

    if (selectedItem && !selectedItem->IsEmpty() && dynamic_cast<BoatItem *>(selectedItem->GetItem()))
    {
        delete hit;
        hit = m_level->Clip(GetEyePosition(Everstead::GetInstance()->GetRenderPartialTicks()), glm::normalize(m_front), (float) GetBlockReach(),
                            false, true);
        if (hit->IsBlock() && m_level->GetWorldBorder().IsWithinBounds(hit->GetBlockPos()))
        {
            Swing();
            UseItemOn(hit->GetBlockPos(), hit->GetBlockFace());
        }

        delete hit;
        return;
    }

    if (block && GameType::MayBuild(m_gameType) && dynamic_cast<LilyPadBlock *>(block))
    {
        delete hit;
        hit = m_level->Clip(GetEyePosition(Everstead::GetInstance()->GetRenderPartialTicks()), glm::normalize(m_front), (float) GetBlockReach(),
                            false, true);
        if (hit->IsBlock())
        {
            PlaceHeldBlock(hit, selectedItem, block);
        }

        delete hit;
        return;
    }

    FlintAndSteelItem *flintAndSteel =
            (selectedItem && !selectedItem->IsEmpty()) ? dynamic_cast<FlintAndSteelItem *>(selectedItem->GetItem()) : nullptr;
    if (flintAndSteel)
    {
        if (hit->IsBlock() && flintAndSteel->CanUse(m_level, hit->GetBlockPos(), hit->GetBlockFace()))
        {
            Swing();
            UseItemOn(hit->GetBlockPos(), hit->GetBlockFace());
            if (!GameType::HasInfiniteItems(GetGameType()))
            {
                selectedItem->ConsumeOne();
            }
        }

        delete hit;
        return;
    }

    SpawnEggItem *spawnEgg = (selectedItem && !selectedItem->IsEmpty()) ? dynamic_cast<SpawnEggItem *>(selectedItem->GetItem()) : nullptr;
    if (spawnEgg)
    {
        if (hit->IsBlock())
        {
            Swing();
            UseItemOn(hit->GetBlockPos(), hit->GetBlockFace());
            if (!GameType::HasInfiniteItems(GetGameType()))
            {
                selectedItem->ConsumeOne();
            }
        }

        delete hit;
        return;
    }

    if (hit->IsBlock() && selectedItem && !selectedItem->IsEmpty() &&
        selectedItem->GetItem()->CanUseOn(m_level, hit->GetBlockPos(), hit->GetBlockFace()))
    {
        Swing();
        selectedItem->GetItem()->SpawnUseEffects(m_level, hit->GetBlockPos());
        UseItemOn(hit->GetBlockPos(), hit->GetBlockFace());
        if (selectedItem->GetItem()->IsDamageable() && !GameType::HasInfiniteItems(GetGameType()))
        {
            selectedItem->ConsumeOne();
        }

        delete hit;
        return;
    }

    if (canPlace && anim != eUseAnim_Eat)
    {
        PlaceHeldBlock(hit, selectedItem, block);
    }

    delete hit;
}

bool LocalPlayer::PlaceHeldBlock(HitResult *hit, ItemInstance *selectedItem, Block *block)
{
    BlockPos placedPos;
    if (!PlaceBlock(hit->GetBlockPos(), hit->GetBlockFace(), hit->GetHitPos(), block, &placedPos))
    {
        return false;
    }

    Swing();
    m_level->PlayBlockPlaceSound(placedPos, block);
    if (!GameType::HasInfiniteItems(GetGameType()))
    {
        selectedItem->ConsumeOne();
    }

    return true;
}

static bool PlacesBlock(const ItemInstance *item, Block *block)
{
    return item && !item->IsEmpty() && item->GetItem()->GetBlock() == block;
}

void LocalPlayer::PickTargetedBlock(bool copyData)
{
    HitResult *hit = Pick();
    BlockPos pos   = hit->GetBlockPos();
    Block *block   = hit->IsBlock() ? Block::ById(m_level->GetBlockId(pos)) : nullptr;
    delete hit;
    if (!block || GameType::IsSpectator(m_gameType))
    {
        return;
    }

    if (block == Block::ByName("bamboo"))
    {
        block = Block::ByName("bamboo_sapling");
    }
    else if (dynamic_cast<SignBlock *>(block))
    {
        block = ((SignBlock *) block)->GetStandingBlock();
    }

    Inventory *inventory = GetInventory();

    // This picks the first empty hotbar slot starting at the selected one, otherwise the selected slot
    int selected   = inventory->GetSelectedSlot();
    int hotbarSlot = selected;
    for (int i = 0; i < Inventory::s_hotbarSize; i++)
    {
        int candidate = (selected + i) % Inventory::s_hotbarSize;
        if (inventory->GetItem(candidate)->IsEmpty())
        {
            hotbarSlot = candidate;
            break;
        }
    }

    ClientConnection *connection = ((MultiPlayerLocalPlayer *) this)->GetClientConnection();
    if (copyData && GameType::HasInfiniteItems(m_gameType) && m_level->GetBlockEntity(pos))
    {
        inventory->SetSelectedSlot(hotbarSlot);
        if (connection)
        {
            ((MultiPlayerLocalPlayer *) this)->SendCarriedItemIfNeeded();
            connection->Send(std::shared_ptr<Packet>(new PickBlockC2SPacket(pos, (uint8_t) hotbarSlot)));
        }

        return;
    }

    for (int i = 0; i < Inventory::s_hotbarSize; i++)
    {
        if (PlacesBlock(inventory->GetItem(i), block))
        {
            inventory->SetSelectedSlot(i);
            return;
        }
    }

    uint8_t menuId   = InventoryMenu::s_playerMenuId;
    eCLICK_TYPE type = eClickType_Swap;
    int slot         = -1;
    for (int i = Inventory::s_hotbarSize; i < Inventory::s_totalSize && slot < 0; i++)
    {
        if (PlacesBlock(inventory->GetSlot(i), block))
        {
            slot = i;
        }
    }

    const std::vector<ItemInstance> &creativeItems = CreativeTabs::GetItems(CreativeTabs::s_allItemsTab);
    for (size_t i = 0; i < creativeItems.size() && slot < 0 && GameType::HasInfiniteItems(m_gameType); i++)
    {
        if (PlacesBlock(&creativeItems[i], block))
        {
            menuId = InventoryMenu::s_creativeMenuId;
            type   = eClickType_PickBlock;
            slot   = InventoryMenu::GetCreativeSlot(CreativeTabs::s_allItemsTab, (int) i);
        }
    }

    if (slot < 0)
    {
        return;
    }

    inventory->SetSelectedSlot(hotbarSlot);

    // Clicks are applied locally and mirrored to the host, the same way the inventory screens do it
    InventoryMenu(inventory, nullptr, menuId).Click(slot, hotbarSlot, type);
    if (connection)
    {
        // The host echoes the inventory with its selected slot, so it has to learn the new slot first
        ((MultiPlayerLocalPlayer *) this)->SendCarriedItemIfNeeded();
        connection->Send(std::shared_ptr<Packet>(new ContainerClickC2SPacket(menuId, (int16_t) slot, (uint8_t) hotbarSlot, type)));
    }
}

void LocalPlayer::ScrollFlySpeed(int steps)
{
    AttributeInstance *flySpeed = m_attributes.Get(Attribute::s_flyingSpeed);
    flySpeed->SetBaseValue(std::clamp(flySpeed->GetBaseValue() + s_flySpeedStep * (double) steps, 0.0, s_maxFlySpeed));
}

void LocalPlayer::DropSelectedItem(bool all)
{
    if (GameType::IsSpectator(m_gameType))
    {
        return;
    }

    Inventory *inventory = GetInventory();
    int slot             = inventory->GetSelectedSlot();
    if (inventory->GetItem(slot)->IsEmpty())
    {
        return;
    }

    // The hotbar sits at the start of the player slots so the selected slot is also its menu slot
    int button = all ? InventoryMenu::s_rightButton : InventoryMenu::s_leftButton;
    InventoryMenu menu(inventory, nullptr, InventoryMenu::s_playerMenuId);
    menu.Click(slot, button, eClickType_Throw);
    ClientConnection *connection = ((MultiPlayerLocalPlayer *) this)->GetClientConnection();
    if (connection)
    {
        connection->Send(std::shared_ptr<Packet>(
                new ContainerClickC2SPacket(InventoryMenu::s_playerMenuId, (int16_t) slot, (uint8_t) button, eClickType_Throw)));
    }
}

void LocalPlayer::OpenContainer(AbstractContainer *container)
{
    Everstead *everstead = Everstead::GetInstance();
    if (container != m_recipeView)
    {
        everstead->ClosePauseMenu();
    }

    Player::OpenContainer(container);

    if (container == m_craftingContainer)
    {
        everstead->GetUiController()->OpenScene(eUIScene_CraftingTable);
    }
    else if (container == m_recipeView)
    {
        // The cursor is already free in the screen below, so we leave it where it is
        everstead->GetUiController()->NavigateToScene(eUIScene_RecipeView);
        ClearInputs();
        everstead->GetInputManager()->Clear();
        return;
    }
    else if (container == &m_furnaceContainer)
    {
        everstead->GetUiController()->OpenScene(eUIScene_Furnace);
    }
    else if (container == &m_resonantSocketContainer)
    {
        everstead->GetUiController()->OpenScene(eUIScene_ResonantSocket);
    }
    else if (container == &m_resonatorContainer)
    {
        everstead->GetUiController()->OpenScene(eUIScene_Resonator);
    }
    else if (container == m_anvilContainer)
    {
        everstead->GetUiController()->OpenScene(eUIScene_Anvil);
    }
    else
    {
        everstead->GetUiController()->OpenScene(eUIScene_Container);
    }

    ClearInputs();
    everstead->GetInputManager()->Clear();
    everstead->SetMouseLock(false);
}

void LocalPlayer::CloseContainer()
{
    Everstead *everstead = Everstead::GetInstance();
    if (GetOpenContainer() == m_recipeView)
    {
        // The recipe view sits on top of the screen it was opened from, so closing it goes back there
        Player::OpenContainer(m_recipeViewParent);
        everstead->GetUiController()->CloseScene();
        ClearInputs();
        everstead->GetInputManager()->Clear();
        return;
    }

    Player::CloseContainer();
    everstead->GetUiController()->CloseScene();
    ClearInputs();
    everstead->GetInputManager()->Clear();
    everstead->SetMouseLock(true);
}

void LocalPlayer::ViewRecipe(const Item *item)
{
    std::vector<RecipeViewPage> pages;
    std::vector<const Recipe *> recipes = Recipes::GetInstance()->FindRecipes(item);
    for (size_t i = 0; i < recipes.size(); i++)
    {
        pages.push_back({eRecipeViewType_Crafting, recipes[i], nullptr, ItemInstance(), ItemInstance(), false});
    }

    std::vector<Item *> inputs = FurnaceRecipes::GetInstance()->FindInputs(item);
    for (size_t i = 0; i < inputs.size(); i++)
    {
        pages.push_back({eRecipeViewType_Furnace, nullptr, inputs[i], ItemInstance(), ItemInstance(), false});
    }

    const std::string &path = item->GetResourceLocation().GetPath();
    Item *gear              = path.rfind("severite_", 0) == 0 ? Item::ByName("diamond_" + path.substr(9)) : nullptr;
    Item *upgrade           = gear ? ResonatorContainer::GetUpgradeResult(ItemInstance(gear)) : nullptr;
    if (upgrade == item)
    {
        pages.push_back({eRecipeViewType_Resonator, nullptr, nullptr, ItemInstance(gear), ItemInstance(upgrade), false});
    }

    std::vector<ItemInstance> drops;
    BlockRegistry *blocks = BlockRegistry::GetInstance();
    for (uint32_t i = 0; i < blocks->Size(); i++)
    {
        Block *block           = blocks->ById(i);
        Item *blockItem        = Item::ByName(block->GetResourceLocation());
        const LootTable *table = LootTables::ForBlock(block);
        if (!blockItem || !table || blockItem == item)
        {
            continue;
        }

        drops.clear();
        table->GetPossibleItems(&drops);
        for (size_t j = 0; j < drops.size(); j++)
        {
            if (drops[j].GetItem() == item)
            {
                pages.push_back({eRecipeViewType_Break, nullptr, nullptr, ItemInstance(blockItem, 1, 0), drops[j], false});
                break;
            }
        }
    }

    const std::map<std::string, LootTable> &entityTables = LootTables::GetEntityTables();
    for (auto it = entityTables.begin(); it != entityTables.end(); it++)
    {
        Item *egg = Item::ByName(it->first + "_spawn_egg");
        if (!egg)
        {
            continue;
        }

        drops.clear();
        it->second.GetPossibleItems(&drops);
        for (size_t j = 0; j < drops.size(); j++)
        {
            if (drops[j].GetItem() == item)
            {
                pages.push_back({eRecipeViewType_Break, nullptr, nullptr, ItemInstance(egg), drops[j], true});
                break;
            }
        }
    }

    if (pages.empty())
    {
        return;
    }

    if (GetOpenContainer() != m_recipeView)
    {
        m_recipeViewParent = GetOpenContainer();
    }

    m_recipeViewPages = pages;
    SetRecipeViewPage(0);
}

void LocalPlayer::SetRecipeViewPage(int page)
{
    // The scene lays out its slots for one recipe type, so every page change rebuilds it
    if (GetOpenContainer() == m_recipeView)
    {
        Everstead::GetInstance()->GetUiController()->CloseScene();
    }

    m_recipeViewPage    = page;
    m_recipeViewStartMs = Time::GetMilliseconds();
    FillRecipeView();
    OpenContainer(m_recipeView);
}

void LocalPlayer::FillRecipeView()
{
    // We step through every ingredient choice once a second
    int cycle = (int) ((Time::GetMilliseconds() - m_recipeViewStartMs) / 1000);
    for (int slot = 0; slot < m_recipeView->GetSize(); slot++)
    {
        m_recipeView->SetItem(slot, ItemInstance());
    }

    const RecipeViewPage &page = m_recipeViewPages[(size_t) m_recipeViewPage];
    if (page.type == eRecipeViewType_Resonator)
    {
        m_recipeView->SetItem(CraftingContainer::s_gridSlot, page.source);
        m_recipeView->SetItem(CraftingContainer::s_gridSlot + 1, ItemInstance(Item::ByName("severite")));
        m_recipeView->SetItem(CraftingContainer::s_resultSlot, page.result);
        return;
    }

    if (page.type == eRecipeViewType_Break)
    {
        m_recipeView->SetItem(CraftingContainer::s_gridSlot, page.source);
        m_recipeView->SetItem(CraftingContainer::s_resultSlot, page.result);
        return;
    }

    if (page.type == eRecipeViewType_Furnace)
    {
        FurnaceRecipes *furnaceRecipes   = FurnaceRecipes::GetInstance();
        const std::vector<Item *> &fuels = furnaceRecipes->GetFuels();
        ItemInstance input(page.input);
        m_recipeView->SetItem(CraftingContainer::s_gridSlot, input);
        m_recipeView->SetItem(CraftingContainer::s_gridSlot + 1, ItemInstance(fuels[(size_t) cycle % fuels.size()]));
        m_recipeView->SetItem(CraftingContainer::s_resultSlot, furnaceRecipes->GetResult(input));
        return;
    }

    m_recipeView->SetItem(CraftingContainer::s_resultSlot, page.recipe->GetResult());
    page.recipe->FillGrid(m_recipeView, cycle);
}

CraftingContainer *LocalPlayer::GetRecipeView() const
{
    return m_recipeView;
}

eRECIPE_VIEW_TYPE LocalPlayer::GetRecipeViewType() const
{
    return m_recipeViewPages[(size_t) m_recipeViewPage].type;
}

bool LocalPlayer::IsKillRecipeView() const
{
    return m_recipeViewPages[(size_t) m_recipeViewPage].kill;
}

int LocalPlayer::GetRecipeViewPage() const
{
    return m_recipeViewPage;
}

int LocalPlayer::GetRecipeViewPageCount() const
{
    return (int) m_recipeViewPages.size();
}

FurnaceContainer *LocalPlayer::GetFurnaceContainer()
{
    return &m_furnaceContainer;
}

ResonantSocketContainer *LocalPlayer::GetResonantSocketContainer()
{
    return &m_resonantSocketContainer;
}

ResonatorContainer *LocalPlayer::GetResonatorContainer()
{
    return &m_resonatorContainer;
}

void LocalPlayer::OpenChest(uint8_t menuId, int rows)
{
    m_chestMenuId = menuId;
    delete m_chestContainer;
    int columns = 9;
    if (menuId == InventoryMenu::s_hopperMenuId)
    {
        columns = 5;
    }
    else if (menuId == InventoryMenu::s_dispenserMenuId || menuId == InventoryMenu::s_dropperMenuId)
    {
        columns = 3;
    }

    m_chestContainer = new Container(rows, columns);
    OpenContainer(m_chestContainer);
}

uint8_t LocalPlayer::GetChestMenuId() const
{
    return m_chestMenuId;
}

void LocalPlayer::CycleThirdPersonView()
{
    if (m_thirdPersonView == eThirdPersonView_FirstPerson)
    {
        m_thirdPersonView = eThirdPersonView_Back;
    }
    else if (m_thirdPersonView == eThirdPersonView_Back)
    {
        m_thirdPersonView = eThirdPersonView_Front;
    }
    else
    {
        m_thirdPersonView = eThirdPersonView_FirstPerson;
    }
}

eTHIRD_PERSON_VIEW LocalPlayer::GetThirdPersonView() const
{
    return m_thirdPersonView;
}

bool LocalPlayer::IsFirstPerson() const
{
    return m_thirdPersonView == eThirdPersonView_FirstPerson;
}

void LocalPlayer::PlaySound(const ResourceLocation &location, float volume, float pitch)
{
    m_level->PlaySound(location, volume, pitch);
}

void LocalPlayer::WearHeldItem(const BlockPos &pos, Block *block)
{
    if (!block->CanSurvive(m_level, pos, block->GetAttachment(m_level->GetBlockData(pos))))
    {
        return;
    }

    ItemInstance *held = GetInventory()->GetSelectedItem();
    int damage         = held->IsEmpty() ? 0 : held->GetItem()->GetMineDamage(block);
    for (int i = 0; i < damage && !held->IsEmpty(); i++)
    {
        held->ConsumeOne();
    }
}

void LocalPlayer::DestroyBlock(const BlockPos &pos)
{
    SetBlockByPlayer(pos, Block::ById(0), nullptr, 0);

    Block *air    = Block::ById(0);
    Block *bamboo = Block::ByName("bamboo");
    std::vector<BlockPos> pending;
    pending.push_back(pos);
    while (!pending.empty())
    {
        BlockPos current = pending.back();
        pending.pop_back();

        BlockPos neighbors[] = {BlockPos(current.x - 1, current.y, current.z), BlockPos(current.x + 1, current.y, current.z),
                                BlockPos(current.x, current.y - 1, current.z), BlockPos(current.x, current.y + 1, current.z),
                                BlockPos(current.x, current.y, current.z - 1), BlockPos(current.x, current.y, current.z + 1)};
        for (int i = 0; i < 6; i++)
        {
            Block *neighbor = Block::ById(m_level->GetBlockId(neighbors[i]));
            if (!neighbor || neighbor == air || neighbor == bamboo ||
                neighbor->CanSurvive(m_level, neighbors[i], neighbor->GetAttachment(m_level->GetBlockData(neighbors[i]))))
            {
                continue;
            }

            SpawnDestroyParticlesAt(neighbors[i]);
            m_level->PlayBlockBreakSound(neighbors[i], neighbor);
            SetBlockByPlayer(neighbors[i], air, nullptr, 0);
            pending.push_back(neighbors[i]);
        }
    }
}

Direction *LocalPlayer::GetParticleDirection(Block *block) const
{
    Direction *directions[] = {Direction::north, Direction::south, Direction::west, Direction::east, Direction::up, Direction::down};

    for (Direction *direction : directions)
    {
        if (!block->GetTexturePath(direction).empty())
        {
            return direction;
        }
    }

    return Direction::north;
}

uint32_t LocalPlayer::MultiplyColorRgb(uint32_t color, uint32_t tint) const
{
    uint32_t a  = (color >> 24) & 0xFF;
    uint32_t r  = (color >> 16) & 0xFF;
    uint32_t g  = (color >> 8) & 0xFF;
    uint32_t b  = color & 0xFF;
    uint32_t tr = (tint >> 16) & 0xFF;
    uint32_t tg = (tint >> 8) & 0xFF;
    uint32_t tb = tint & 0xFF;

    r = (r * tr + 127) / 255;
    g = (g * tg + 127) / 255;
    b = (b * tb + 127) / 255;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

ParticleSpawnParams LocalPlayer::GetTerrainParticleParams(const BlockPos &pos, Block *block, const Chunk *chunk) const
{
    Direction *particleDirection = GetParticleDirection(block);
    Block::UVRect atlasRect      = block->GetAtlasUVRect(particleDirection);
    ParticleSpawnParams params;
    params.type        = eParticleType_Terrain;
    params.region      = {atlasRect.u0, atlasRect.v0, atlasRect.u1, atlasRect.v1};
    params.color       = 0xFF999999;
    params.hasVelocity = true;
    params.hasRegion   = true;
    params.hasColor    = true;
    params.color       = MultiplyColorRgb(params.color, block->GetDataTint(m_level->GetBlockData(pos)));
    if (chunk)
    {
        params.color = MultiplyColorRgb(params.color, block->ResolveTint(particleDirection, m_level, chunk, Mth::FloorMod(pos.x, Chunk::s_sizeX),
                                                                         Mth::FloorMod(pos.z, Chunk::s_sizeZ)));
    }

    return params;
}

void LocalPlayer::SpawnDestroyParticles(const BlockPos &pos, const AABB &aabb, Block *block, const Chunk *chunk)
{
    if (!block)
    {
        return;
    }

    const glm::dvec3 &min      = aabb.GetMin();
    const glm::dvec3 &max      = aabb.GetMax();
    double dx                  = std::min(1.0, max.x - min.x);
    double dy                  = std::min(1.0, max.y - min.y);
    double dz                  = std::min(1.0, max.z - min.z);
    int sx                     = std::max(2, (int) std::ceil(dx / 0.25));
    int sy                     = std::max(2, (int) std::ceil(dy / 0.25));
    int sz                     = std::max(2, (int) std::ceil(dz / 0.25));
    ParticleSpawnParams params = GetTerrainParticleParams(pos, block, chunk);
    for (int x = 0; x < sx; x++)
    {
        for (int y = 0; y < sy; y++)
        {
            for (int z = 0; z < sz; z++)
            {
                double fx       = ((double) x + 0.5) / (double) sx;
                double fy       = ((double) y + 0.5) / (double) sy;
                double fz       = ((double) z + 0.5) / (double) sz;
                params.position = glm::dvec3(min.x + fx * dx, min.y + fy * dy, min.z + fz * dz);
                params.velocity = glm::dvec3(fx - 0.5, fy - 0.5, fz - 0.5);
                m_level->SpawnParticle(params);
            }
        }
    }
}

bool LocalPlayer::PlaceBlock(const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, Block *block, BlockPos *outPlacedPos)
{
    BlockPos placePos                = pos;
    Block *targetBlock               = Block::ById(m_level->GetBlockId(pos));
    PointedDripstoneBlock *dripstone = dynamic_cast<PointedDripstoneBlock *>(block);
    if (dripstone && targetBlock == block && face->dy == 0)
    {
        face = dripstone->GetTipDirection(m_level->GetBlockData(pos));
        while (placePos.y + face->dy >= 0 && placePos.y + face->dy < Chunk::s_sizeY &&
               dripstone->IsPointingAt(m_level, BlockPos(placePos.x, placePos.y + face->dy, placePos.z), face))
        {
            placePos.y += face->dy;
        }

        return PlaceBlock(placePos, face, hitPos, block, outPlacedPos);
    }

    bool placeAtTarget = false;
    if (dynamic_cast<ScaffoldingBlock *>(block) && targetBlock == block && !IsSneaking())
    {
        Direction *direction = Direction::up;
        if (face == Direction::up)
        {
            if (fabs(m_front.x) > fabs(m_front.z))
            {
                direction = m_front.x > 0.0 ? Direction::east : Direction::west;
            }
            else
            {
                direction = m_front.z > 0.0 ? Direction::south : Direction::north;
            }
        }

        int steps = 0;
        while (steps < 7 && placePos.y + direction->dy < Chunk::s_sizeY)
        {
            BlockPos next(placePos.x + direction->dx, placePos.y + direction->dy, placePos.z + direction->dz);
            placePos = next;
            if (Block::ById(m_level->GetBlockId(next)) != block)
            {
                face          = direction;
                placeAtTarget = true;
                break;
            }

            if (direction->dy == 0)
            {
                steps++;
            }
        }

        if (!placeAtTarget)
        {
            return false;
        }
    }

    if (!placeAtTarget && targetBlock == block && (face == Direction::up || face == Direction::down))
    {
        uint16_t combineData = block->GetPlacementState(m_level, pos, Direction::Opposite(face), hitPos, m_front);
        Block *combined      = block->GetCombinedBlock(m_level->GetBlockData(pos), combineData);
        if (combined)
        {
            SetBlockByPlayer(pos, combined, face, combineData);
            if (outPlacedPos)
            {
                *outPlacedPos = pos;
            }

            return true;
        }
    }

    if (!placeAtTarget && (!targetBlock || targetBlock == block || !targetBlock->CanBeReplaced(m_level->GetBlockData(pos)) ||
                           (dynamic_cast<LilyPadBlock *>(block) && targetBlock->IsLiquid())))
    {
        if (face == Direction::west)
        {
            placePos.x--;
        }

        if (face == Direction::east)
        {
            placePos.x++;
        }

        if (face == Direction::down)
        {
            placePos.y--;
        }

        if (face == Direction::up)
        {
            placePos.y++;
        }

        if (face == Direction::north)
        {
            placePos.z--;
        }

        if (face == Direction::south)
        {
            placePos.z++;
        }
    }

    if (!m_level->GetWorldBorder().IsWithinBounds(placePos))
    {
        return false;
    }

    uint16_t placeData  = block->GetPlacementState(m_level, placePos, face, hitPos, m_front);
    Block *currentBlock = Block::ById(m_level->GetBlockId(placePos));
    if (currentBlock == block && block->GetCombinedBlock(m_level->GetBlockData(placePos), placeData))
    {
        SetBlockByPlayer(placePos, block->GetCombinedBlock(m_level->GetBlockData(placePos), placeData), face, placeData);
        if (outPlacedPos)
        {
            *outPlacedPos = placePos;
        }

        return true;
    }

    // Replacing a block with itself changes nothing, so it counts as a failed placement
    if (currentBlock == block ||
        (currentBlock && currentBlock != Block::ByName("air") && !currentBlock->CanBeReplaced(m_level->GetBlockData(placePos))))
    {
        return false;
    }

    Block *placedBlock = block->GetPlacementBlock(face);
    if (!placedBlock || !placedBlock->CanSurvive(m_level, placePos, placedBlock->GetAttachment(placedBlock->GetPlacementData(face))))
    {
        return false;
    }

    if (placedBlock != block)
    {
        placeData = placedBlock->GetPlacementState(m_level, placePos, face, hitPos, m_front);
    }

    if (placedBlock->IsCollidable())
    {
        std::vector<AABB> boxes;
        placedBlock->GetCollisionBoxes(placeData, &boxes);
        const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
        for (size_t i = 0; i < boxes.size(); i++)
        {
            AABB blockBox = boxes[i].Translated(glm::dvec3(placePos.x, placePos.y, placePos.z));
            if (blockBox.Intersects(GetAABB()))
            {
                return false;
            }

            for (size_t j = 0; j < entities.size(); j++)
            {
                glm::dvec3 entityPos = entities[j]->GetPosition();
                if (entities[j]->GetType() == eEntityType_Minecart)
                {
                    entityPos.y -= Minecart::kClientBoxOffset;
                }

                if (entities[j]->BlocksBuilding() && blockBox.Intersects(entities[j]->GetBoundingBox().Translated(entityPos)))
                {
                    return false;
                }
            }
        }
    }

    BedBlock *bed = dynamic_cast<BedBlock *>(placedBlock);
    if (bed && !bed->CanPlaceAt(m_level, placePos, placeData))
    {
        return false;
    }

    CropBlock *crop = dynamic_cast<CropBlock *>(placedBlock);
    StemBlock *stem = dynamic_cast<StemBlock *>(placedBlock);
    if ((crop && !crop->CanPlaceAt(m_level, placePos)) || (stem && !stem->CanPlaceAt(m_level, placePos)))
    {
        return false;
    }

    SetBlockByPlayer(placePos, placedBlock, face, placeData);

    if (outPlacedPos)
    {
        *outPlacedPos = placePos;
    }

    return true;
}

void LocalPlayer::SetBlockByPlayer(const BlockPos &pos, Block *block, Direction *placedAgainst, uint16_t data)
{
    Block *current = Block::ById(m_level->GetBlockId(pos));
    if (current != block && current->GetCombinedBlock(m_level->GetBlockData(pos), data) == block)
    {
        m_level->SetBlock(pos, block, placedAgainst, 0);
        return;
    }

    m_level->SetBlock(pos, block, placedAgainst, data);
}

bool LocalPlayer::IsOpenBlockSpace(const BlockPos &pos) const
{
    Block *block = Block::ById(m_level->GetBlockId(pos));
    return !block->IsSolid() || block->IsLiquid() || block->GetRenderShape() != Block::eRenderShape_CUBE;
}

bool LocalPlayer::IsHeadspaceFree(const BlockPos &pos, int height) const
{
    for (int y = 0; y < height; y++)
    {
        if (!IsOpenBlockSpace(BlockPos(pos.x, pos.y + y, pos.z)))
        {
            return false;
        }
    }

    return true;
}

// This nudges the player toward the nearest open side when a corner is inside a full block
void LocalPlayer::PushOutOfBlock(double x, double y, double z)
{
    BlockPos pos((int) floor(x), (int) floor(y), (int) floor(z));
    int height = std::max((int) std::round(m_boundingBox.GetMax().y - m_boundingBox.GetMin().y), 1);
    if (IsHeadspaceFree(pos, height))
    {
        return;
    }

    double fractionX = x - (double) pos.x;
    double fractionZ = z - (double) pos.z;
    double closest   = 9999.0;
    int side         = -1;
    if (IsHeadspaceFree(BlockPos(pos.x - 1, pos.y, pos.z), height) && fractionX < closest)
    {
        closest = fractionX;
        side    = 0;
    }

    if (IsHeadspaceFree(BlockPos(pos.x + 1, pos.y, pos.z), height) && 1.0 - fractionX < closest)
    {
        closest = 1.0 - fractionX;
        side    = 1;
    }

    if (IsHeadspaceFree(BlockPos(pos.x, pos.y, pos.z - 1), height) && fractionZ < closest)
    {
        closest = fractionZ;
        side    = 2;
    }

    if (IsHeadspaceFree(BlockPos(pos.x, pos.y, pos.z + 1), height) && 1.0 - fractionZ < closest)
    {
        side = 3;
    }

    if (side == 0)
    {
        m_velocity.x = -0.1;
    }
    else if (side == 1)
    {
        m_velocity.x = 0.1;
    }
    else if (side == 2)
    {
        m_velocity.z = -0.1;
    }
    else if (side == 3)
    {
        m_velocity.z = 0.1;
    }
}

void LocalPlayer::PushOutOfBlocks()
{
    if (m_noCollision)
    {
        return;
    }

    double inset = (m_boundingBox.GetMax().x - m_boundingBox.GetMin().x) * 0.35;
    double y     = m_aabb.GetMin().y + 0.5;
    PushOutOfBlock(m_position.x - inset, y, m_position.z + inset);
    PushOutOfBlock(m_position.x - inset, y, m_position.z - inset);
    PushOutOfBlock(m_position.x + inset, y, m_position.z - inset);
    PushOutOfBlock(m_position.x + inset, y, m_position.z + inset);
}

float LocalPlayer::GetCaveMood() const
{
    return m_caveMood;
}

int LocalPlayer::GetCaveSoundsPlayed() const
{
    return m_caveSoundsPlayed;
}

void LocalPlayer::UpdateCaveMood()
{
    const int searchExtent = 8;

    int searchSize = searchExtent * 2 + 1;
    glm::dvec3 eye = m_position + glm::dvec3(0.0, GetEyeHeight(), 0.0);
    BlockPos pos((int) std::floor(eye.x) + m_caveRandom.NextInt(0, searchSize - 1) - searchExtent,
                 (int) std::floor(eye.y) + m_caveRandom.NextInt(0, searchSize - 1) - searchExtent,
                 (int) std::floor(eye.z) + m_caveRandom.NextInt(0, searchSize - 1) - searchExtent);
    if (!m_level->GetChunk(ChunkPos(pos.x >> 4, pos.z >> 4)) || m_level->GetBlockId(pos) != 0)
    {
        return;
    }

    int skyLight = m_level->GetSkyLightLevel(pos);
    if (skyLight > 0)
    {
        m_caveMood -= (float) skyLight / 15.0f * 0.001f;
    }
    else
    {
        m_caveMood -= (float) (m_level->GetBlockLightLevel(pos) - 1) / s_caveMoodTickDelay;
    }

    if (m_caveMood < 1.0f)
    {
        m_caveMood = std::max(m_caveMood, 0.0f);
        return;
    }

    m_level->PlaySound("ambient.cave.cave" + std::to_string(m_caveRandom.NextInt(1, s_caveSoundCount)),
                       glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f, 0.5f + m_caveRandom.NextFloat() * 0.5f);
    m_caveSoundsPlayed++;
    m_caveMood = 0.0f;
}

#endif
