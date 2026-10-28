#include "ServerPlayerGameMode.h"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "../../core/EversteadServer.h"
#include "../../core/GameInfo.h"
#include "../../network/PlayerList.h"
#include "../../network/listener/PlayerConnection.h"
#include "../../network/packet/s2c/BlockDestructionS2CPacket.h"
#include "../../network/packet/s2c/BlockUpdateS2CPacket.h"
#include "../../network/packet/s2c/OpenSignEditorS2CPacket.h"
#include "../../network/packet/s2c/ParticleS2CPacket.h"
#include "../../network/packet/s2c/SignDataS2CPacket.h"
#include "../../network/packet/s2c/SoundS2CPacket.h"
#include "../../network/packet/s2c/SpawnerDataS2CPacket.h"
#include "../../tag/CompoundTag.h"
#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../../utils/math/Mth.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/AnvilBlock.h"
#include "../../world/block/BedBlock.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRegistry.h"
#include "../../world/block/ChestBlock.h"
#include "../../world/block/CraftingTableBlock.h"
#include "../../world/block/CropBlock.h"
#include "../../world/block/EchoGlassBlock.h"
#include "../../world/block/FenceBlock.h"
#include "../../world/block/LilyPadBlock.h"
#include "../../world/block/SignBlock.h"
#include "../../world/block/StemBlock.h"
#include "../../world/block/WallBlock.h"
#include "../../world/blockentity/MobSpawnerBlockEntity.h"
#include "../../world/blockentity/PistonMovingBlockEntity.h"
#include "../../world/blockentity/SignBlockEntity.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/fracture/FractureTraits.h"
#include "../../world/inventory/InventoryMenu.h"
#include "../../world/inventory/ServerContainerMenu.h"
#include "../../world/item/BoatItem.h"
#include "../../world/item/BowItem.h"
#include "../../world/item/BucketItem.h"
#include "../../world/item/FlintAndSteelItem.h"
#include "../../world/item/Item.h"
#include "../../world/item/SpawnEggItem.h"
#include "../../world/item/SwordItem.h"
#include "../../world/item/ThrowableItem.h"
#include "../item/LeashKnot.h"
#include "ServerPlayer.h"

ServerPlayerGameMode::ServerPlayerGameMode(ServerPlayer *player)
{
    m_player            = player;
    m_isDestroying      = false;
    m_destroyPos        = BlockPos(0, 0, 0);
    m_destroyProgress   = 0.0f;
    m_hasDelayedDestroy = false;
    m_lastDestroyStage  = -1;
    m_tickCount         = 0;
}

void ServerPlayerGameMode::Tick()
{
    if (m_isDestroying)
    {
        Level *level     = m_player->GetLevel();
        uint32_t blockId = level->GetBlockId(m_destroyPos);
        if (blockId == 0)
        {
            StopDestroy();
        }
        else
        {
            Block *block = Block::ById(blockId);
            Block *broken =
                    block == Block::ByName("moving_piston") ? ((PistonMovingBlockEntity *) level->GetBlockEntity(m_destroyPos))->GetBlock() : block;
            m_destroyProgress += m_player->GetDestroyProgress(broken);
            if (m_hasDelayedDestroy && block->IsSelectable() && m_destroyProgress >= 0.7f)
            {
                DestroyBlock(level, m_destroyPos);
                m_tickCount++;
                return;
            }

            int stage = std::min((int) (m_destroyProgress * 10.0f), 9);
            if (stage != m_lastDestroyStage)
            {
                m_lastDestroyStage = stage;
                m_player->GetServer()->GetPlayerList()->SendToOthers(
                        m_player, std::shared_ptr<Packet>(new BlockDestructionS2CPacket(m_player->GetPlayerIndex(), m_destroyPos, stage)));
            }
        }
    }

    m_tickCount++;
}

void ServerPlayerGameMode::StartDestroy(Level *level, const BlockPos &pos)
{
    AbortDestroy();
    if (CanInteract(level, pos))
    {
        m_isDestroying     = true;
        m_destroyPos       = pos;
        m_destroyProgress  = 0.0f;
        m_lastDestroyStage = -1;
        Block::ById(level->GetBlockId(pos))->OnAttack(level, pos);
    }
}

void ServerPlayerGameMode::AbortDestroy()
{
    if (m_isDestroying)
    {
        StopDestroy();
    }
}

void ServerPlayerGameMode::StopDestroy()
{
    if (m_hasDelayedDestroy)
    {
        m_hasDelayedDestroy = false;
        ResendBlock(m_player->GetLevel(), m_destroyPos);
    }

    m_isDestroying = false;
    m_player->GetServer()->GetPlayerList()->SendToOthers(
            m_player, std::shared_ptr<Packet>(new BlockDestructionS2CPacket(m_player->GetPlayerIndex(), m_destroyPos, -1)));
}

void ServerPlayerGameMode::DestroyBlock(Level *level, const BlockPos &pos)
{
    if (!CanInteract(level, pos))
    {
        ResendBlock(level, pos);
        return;
    }

    Block *air          = Block::ByName("air");
    Block *fire         = Block::ByName("fire");
    Block *currentBlock = Block::ById(level->GetBlockId(pos));
    bool creative       = GameType::HasInfiniteItems(m_player->GetGameType());
    bool tracked        = m_isDestroying && m_destroyPos == pos;
    // A retracting piston base is a moving piston for a few ticks, so we finish the break once it catches up
    if (!creative && tracked && currentBlock == Block::ByName("moving_piston"))
    {
        m_hasDelayedDestroy = true;
        return;
    }

    if (!currentBlock || currentBlock == air || (!currentBlock->IsSelectable() && currentBlock != fire) ||
        (currentBlock == Block::ByName("bedrock") && !creative) ||
        (currentBlock != fire && creative && dynamic_cast<SwordItem *>(m_player->GetInventory()->GetSelectedItem()->GetItem())))
    {
        ResendBlock(level, pos);
        return;
    }

    bool cascade  = !currentBlock->CanSurvive(level, pos, currentBlock->GetAttachment(level->GetBlockData(pos)));
    float perTick = m_player->GetDestroyProgress(currentBlock);
    // We accept a lower total than the client needs since packets arrive with network jitter
    bool destroyed = perTick >= 1.0f || (tracked && m_destroyProgress + perTick >= 0.7f);
    if (!creative && currentBlock != fire && !cascade && !destroyed)
    {
        if (tracked)
        {
            m_hasDelayedDestroy = true;
            return;
        }

        ResendBlock(level, pos);
        return;
    }

    if (tracked)
    {
        m_hasDelayedDestroy = false;
        StopDestroy();
    }

    if (kStickFight && !creative && m_player->GetServer()->GetStickFight()->OnBlockBroken(m_player, pos))
    {
        ResendBlock(level, pos);
        ResendBlock(level, currentBlock->GetOtherPos(pos, level->GetBlockData(pos)));
        return;
    }

    Random random;
    ItemInstance *held                     = m_player->GetInventory()->GetSelectedItem();
    std::vector<FractureTrait *> fractures = FractureTraits::GetActive(*held);
    for (size_t i = 0; !creative && i < fractures.size(); i++)
    {
        if (fractures[i]->PreventBreak(m_player, level, pos))
        {
            ResendBlock(level, pos);
            return;
        }
    }

    Block *replacement = air;
    if (currentBlock == Block::ByName("ice"))
    {
        Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
        if (below && (below->IsSolid() || below->IsLiquid()))
        {
            replacement = Block::ByName("water_flow");
        }
    }

    uint16_t brokenData = level->GetBlockData(pos);
    BlockPos otherPos   = currentBlock->GetOtherPos(pos, brokenData);
    if (creative && otherPos != pos && Block::ById(level->GetBlockId(otherPos)) == currentBlock)
    {
        level->SetBlockAndDataNoNeighborUpdates(otherPos, air, 0);
        level->RecordBlockChange(otherPos);
    }

    level->SetBlock(pos, replacement, nullptr);
    m_player->CauseFoodExhaustion(FoodData::kExhaustionMine);

    PlayerList *playerList = m_player->GetServer()->GetPlayerList();
    int extraBroken        = 0;
    if (!creative && !kStickFight && currentBlock != fire && replacement == air && (cascade || m_player->CanHarvest(currentBlock)))
    {
        currentBlock->DropAsItem(level, pos, brokenData, held);
        FractureTrait::Harvest harvest = {level, pos, currentBlock, brokenData, held, cascade};
        for (size_t i = 0; i < fractures.size(); i++)
        {
            extraBroken += fractures[i]->OnHarvest(m_player, harvest);
        }
    }

    if (!creative && !cascade && !held->IsEmpty())
    {
        int damage = held->GetItem()->GetMineDamage(currentBlock) + extraBroken;
        held->HurtAndBreak(damage, m_player);
        if (damage > 0)
        {
            playerList->SendInventoryTo(m_player);
        }
    }

    glm::vec3 center((float) pos.x + 0.5f, (float) pos.y + 0.5f, (float) pos.z + 0.5f);
    const std::string &breakSound = currentBlock->GetBreakSound();
    if (currentBlock == fire)
    {
        playerList->SendToOthers(m_player, std::shared_ptr<Packet>(new SoundS2CPacket(
                                                   "random.fizz", 0.5f, 2.6f + (random.NextFloat() - random.NextFloat()) * 0.8f, center)));
    }
    else if (!breakSound.empty())
    {
        const float breakSoundPitch = 0.8f;
        int variant                 = random.NextInt(1, currentBlock->GetBreakSoundVariants());
        playerList->SendToOthers(m_player,
                                 std::shared_ptr<Packet>(new SoundS2CPacket(breakSound + std::to_string(variant), 1.0f, breakSoundPitch, center)));
    }

    // The breaker gets this too because a block update sent earlier in the same tick would otherwise undo its prediction
    playerList->SendToLevel(m_player->GetLevel(),
                            std::shared_ptr<Packet>(new BlockUpdateS2CPacket(pos, (uint16_t) BlockRegistry::GetInstance()->IdOf(replacement), 0)));
}

bool ServerPlayerGameMode::BreakExtraBlock(Level *level, const BlockPos &pos, ItemInstance *held)
{
    Block *block = Block::ById(level->GetBlockId(pos));
    if (!block || block == Block::ByName("air") || !block->IsSelectable() || block->IsLiquid() || block->GetDestroyTime() < 0.0f ||
        block->GetBlockEntityId() || !CanInteract(level, pos) || !m_player->CanHarvest(block))
    {
        return false;
    }

    uint16_t data = level->GetBlockData(pos);
    if (block->GetOtherPos(pos, data) != pos)
    {
        return false;
    }

    level->SetBlock(pos, Block::ByName("air"), nullptr);
    level->RecordBlockChange(pos);
    block->DropAsItem(level, pos, data, held);
    glm::dvec3 center(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5);
    ((ServerLevel *) level)
            ->SendToNearby(center, 64.0, std::shared_ptr<Packet>(new ParticleS2CPacket(eParticleType_Rift, center, glm::dvec3(0.3), 0.05f, 6)));
    return true;
}

void ServerPlayerGameMode::UseBlock(Level *level, const BlockPos &pos, Direction *face)
{
    if (!CanInteract(level, pos))
    {
        return;
    }

    Block *block = Block::ById(level->GetBlockId(pos));
    bool canOpen = !GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0 && !m_player->GetOpenContainer();
    if (dynamic_cast<EchoGlassBlock *>(block))
    {
        if (face && m_player->IsSneaking() && GameType::MayBuild(m_player->GetGameType()) && m_player->GetHealth() > 0)
        {
            ((EchoGlassBlock *) block)->ToggleFace(level, pos, face);
        }
    }
    else if (dynamic_cast<CraftingTableBlock *>(block))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenCraftingTable(pos);
        }
    }
    else if (dynamic_cast<AnvilBlock *>(block))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenAnvil(pos);
        }
    }
    else if (block == Block::ByName("furnace") || block == Block::ByName("lit_furnace"))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenFurnace(level, pos);
        }
    }
    else if (block == Block::ByName("resonant_socket"))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenResonantSocket(level, pos);
        }
    }
    else if (block == Block::ByName("resonator"))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenResonator(level, pos);
        }
    }
    else if (block == Block::ByName("hopper"))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenItemTransfer(level, pos, InventoryMenu::s_hopperMenuId);
        }
    }
    else if (block == Block::ByName("dispenser") || block == Block::ByName("dropper"))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenItemTransfer(
                    level, pos, block == Block::ByName("dispenser") ? InventoryMenu::s_dispenserMenuId : InventoryMenu::s_dropperMenuId);
        }
    }
    else if (block == Block::ByName("chest"))
    {
        if (canOpen)
        {
            m_player->GetContainerMenu()->OpenChest(level, pos, (ChestBlock *) block);
        }
    }
    else if (dynamic_cast<BedBlock *>(block))
    {
        if (!GameType::IsSpectator(m_player->GetGameType()))
        {
            m_player->TrySleep(pos);
        }
    }
    else if (dynamic_cast<FenceBlock *>(block) && !dynamic_cast<WallBlock *>(block))
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0)
        {
            LeashKnot::BindPlayerMobs(m_player, pos);
        }
    }
    else if (dynamic_cast<SignBlock *>(block))
    {
        if (!GameType::IsSpectator(m_player->GetGameType()) && m_player->GetHealth() > 0)
        {
            UseSign(level, pos);
        }
    }
    else if (block && block->IsUsable(level->GetBlockData(pos)))
    {
        block->OnUse(level, pos);
    }
}

void ServerPlayerGameMode::UseSign(Level *level, const BlockPos &pos)
{
    constexpr const char *kDyeNames[] = {"bone_meal",   "orange_dye", "magenta_dye",    "light_blue_dye", "yellow_dye", "lime_dye",
                                         "pink_dye",    "gray_dye",   "light_gray_dye", "cyan_dye",       "purple_dye", "blue_dye",
                                         "cocoa_beans", "green_dye",  "red_dye",        "black_dye"};

    SignBlockEntity *sign  = (SignBlockEntity *) level->GetBlockEntity(pos);
    PlayerList *playerList = m_player->GetServer()->GetPlayerList();
    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    Random random;
    if (sign->IsWaxed())
    {
        level->PlaySound("sign.waxed_interact_fail" + std::to_string(random.NextInt(1, 3)), center, 1.0f, 1.0f);
        return;
    }

    bool front                      = sign->IsFacingFrontText(m_player->GetPosition());
    SignBlockEntity::SignText *text = sign->GetText(front);
    ItemInstance *heldItem          = m_player->GetInventory()->GetSelectedItem();
    std::string itemName            = heldItem->IsEmpty() ? "" : heldItem->GetItem()->GetResourceLocation().GetPath();
    bool applied                    = false;
    for (int i = 0; i < 16; i++)
    {
        if (itemName == kDyeNames[i] && text->color != i)
        {
            text->color = i;
            level->PlaySound("dye.dye1", center, 1.0f, 1.0f);
            applied = true;
        }
    }

    if (itemName == "glow_ink_sac" && !text->glowing)
    {
        text->glowing = true;
        level->PlaySound("ink_sac.ink_sac" + std::to_string(random.NextInt(1, 3)), center, 1.0f, 1.0f);
        applied = true;
    }
    else if (itemName == "ink_sac" && text->glowing)
    {
        text->glowing = false;
        level->PlaySound("ink_sac.ink_sac" + std::to_string(random.NextInt(1, 3)), center, 1.0f, 1.0f);
        applied = true;
    }
    else if (itemName == "honeycomb")
    {
        sign->SetWaxed(true);
        level->PlaySound("honeycomb.wax_on" + std::to_string(random.NextInt(1, 3)), center, 1.0f, 1.0f);
        for (int i = 0; i < 6; i++)
        {
            Direction *direction = Direction::ById((uint8_t) i);
            glm::dvec3 offset(direction->dx, direction->dy, direction->dz);
            glm::dvec3 spread = (glm::dvec3(1.0) - glm::abs(offset)) * 0.25;
            playerList->SendToLevel(level, std::shared_ptr<Packet>(new ParticleS2CPacket(eParticleType_WaxOn, center + offset * 0.55, spread, 0.0f,
                                                                                         random.NextInt(3, 5))));
        }

        applied = true;
    }

    if (applied)
    {
        ((ServerLevel *) level)->MarkBlockChunkDirty(pos);
        std::unique_ptr<CompoundTag> tag(new CompoundTag());
        sign->Save(tag.get());
        playerList->SendToLevel(level, std::shared_ptr<Packet>(new SignDataS2CPacket(pos, std::move(tag))));
        if (!GameType::HasInfiniteItems(m_player->GetGameType()))
        {
            heldItem->ConsumeOne();
        }

        playerList->SendInventoryTo(m_player);
        return;
    }

    OpenSignEditor(level, pos, front);
}

void ServerPlayerGameMode::OpenSignEditor(Level *level, const BlockPos &pos, bool front)
{
    SignBlockEntity *sign = (SignBlockEntity *) level->GetBlockEntity(pos);
    Entity *editor        = level->GetEntityById(sign->GetEditorId());
    glm::dvec3 offset     = m_player->GetPosition() - glm::dvec3((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    if (editor && editor != m_player && glm::dot(offset, offset) <= 64.0)
    {
        return;
    }

    sign->SetEditorId(m_player->GetId());
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new OpenSignEditorS2CPacket(pos, front)));
}

void ServerPlayerGameMode::UseItem()
{
    PlayerList *playerList = m_player->GetServer()->GetPlayerList();
    ItemInstance *held     = m_player->GetInventory()->GetSelectedItem();
    if (held->IsEmpty())
    {
        return;
    }

    int armorSlot = held->GetItem()->GetArmorSlot();
    if (armorSlot >= 0)
    {
        ItemInstance *armor = m_player->GetInventory()->GetArmor(armorSlot);
        if (armor->IsEmpty())
        {
            *armor = *held;
            *held  = ItemInstance();
            m_player->SendArmor(armorSlot);
        }

        playerList->SendInventoryTo(m_player);
        return;
    }

    ThrowableItem *throwable = dynamic_cast<ThrowableItem *>(held->GetItem());
    if (throwable)
    {
        Level *level                    = m_player->GetLevel();
        ThrowableProjectile *projectile = throwable->CreateProjectile(level);
        projectile->Shoot(m_player);
        level->PlaySound("random.bow", m_player->GetPosition(), 0.5f, 0.4f / (level->GetRandom().NextFloat() * 0.4f + 0.8f));
        level->AddEntity(std::unique_ptr<Entity>(projectile));
        if (!GameType::HasInfiniteItems(m_player->GetGameType()))
        {
            held->ConsumeOne();
            playerList->SendInventoryTo(m_player);
        }

        return;
    }

    eUSE_ANIM anim = held->GetItem()->GetUseAnimation();
    if (anim == eUseAnim_Block || anim == eUseAnim_Drink ||
        (anim == eUseAnim_Bow &&
         (GameType::HasInfiniteItems(m_player->GetGameType()) || m_player->GetInventory()->FindItem(Item::ByName("arrow")))) ||
        (anim == eUseAnim_Eat && GameType::CanHurtPlayer(m_player->GetGameType()) && m_player->GetFoodData()->NeedsFood()))
    {
        m_player->StartUsingItem(*held, held->GetItem()->GetUseDuration());
    }
}

void ServerPlayerGameMode::ReleaseUsingItem()
{
    ItemInstance *held = m_player->GetInventory()->GetSelectedItem();
    if (m_player->IsDrawingBow() && !held->IsEmpty() && held->GetItem()->GetUseAnimation() == eUseAnim_Bow)
    {
        ((BowItem *) held->GetItem())->Release(m_player, held);
        m_player->GetServer()->GetPlayerList()->SendInventoryTo(m_player);
    }

    m_player->StopUsingItem();
}

void ServerPlayerGameMode::UseItemOn(Level *level, const BlockPos &pos, Direction *face)
{
    if (!CanInteract(level, pos))
    {
        return;
    }

    PlayerList *playerList = m_player->GetServer()->GetPlayerList();
    ItemInstance *heldItem = m_player->GetInventory()->GetSelectedItem();
    if (heldItem->IsEmpty())
    {
        return;
    }

    Item *item                       = heldItem->GetItem();
    BucketItem *bucket               = dynamic_cast<BucketItem *>(item);
    FlintAndSteelItem *flintAndSteel = dynamic_cast<FlintAndSteelItem *>(item);
    SpawnEggItem *spawnEgg           = dynamic_cast<SpawnEggItem *>(item);
    BoatItem *boat                   = dynamic_cast<BoatItem *>(item);
    if (bucket)
    {
        bucket->Use(level, m_player, pos, face);
        playerList->SendInventoryTo(m_player);
    }
    else if (flintAndSteel)
    {
        flintAndSteel->Use(level, m_player, pos, face);
        playerList->SendInventoryTo(m_player);
    }
    else if (spawnEgg && Block::ById(level->GetBlockId(pos)) == Block::ByName("mob_spawner"))
    {
        ((MobSpawnerBlockEntity *) level->GetBlockEntity(pos))->SetEntityType(spawnEgg->GetEntityType());
        playerList->SendToLevel(m_player->GetLevel(), std::shared_ptr<Packet>(new SpawnerDataS2CPacket(pos, spawnEgg->GetEntityType())));
        if (!GameType::HasInfiniteItems(m_player->GetGameType()))
        {
            heldItem->ConsumeOne();
        }

        playerList->SendInventoryTo(m_player);
    }
    else if (spawnEgg)
    {
        spawnEgg->Use(level, m_player, pos, face);
        playerList->SendInventoryTo(m_player);
    }
    else if (boat)
    {
        boat->Use(level, m_player, pos, face);
        playerList->SendInventoryTo(m_player);
    }
    else if (item->CanUseOn(level, pos, face))
    {
        item->UseOn(level, pos, face, m_player->GetFront());
        ConsumeHeldItem(heldItem);
        playerList->SendInventoryTo(m_player);
    }
}

void ServerPlayerGameMode::PlaceBlock(Level *level, const BlockPos &pos, Direction *face, uint16_t data)
{
    ItemInstance *heldItem = m_player->GetInventory()->GetSelectedItem();
    Block *placeBlock      = heldItem->IsEmpty() ? nullptr : heldItem->GetItem()->GetBlock();
    Block *air             = Block::ByName("air");
    if (!placeBlock || placeBlock == air || !IsChunkLoaded(level, pos) || !level->GetWorldBorder().IsWithinBounds(pos))
    {
        RejectPlace(level, pos);
        return;
    }

    // Scaffolding is placed at the end of the clicked column or row, so we accept it when any piece along that line is in reach
    bool reachable = m_player->CanReach(pos);
    BlockPos back(pos.x - face->dx, pos.y - face->dy, pos.z - face->dz);
    for (int steps = 0; !reachable && placeBlock == Block::ByName("scaffolding") && (face->dy != 0 || steps <= 7) && IsChunkLoaded(level, back) &&
                        Block::ById(level->GetBlockId(back)) == placeBlock;
         steps++)
    {
        reachable = m_player->CanReach(back);
        back      = BlockPos(back.x - face->dx, back.y - face->dy, back.z - face->dz);
    }

    if (!reachable)
    {
        RejectPlace(level, pos);
        return;
    }

    PlayerList *playerList = m_player->GetServer()->GetPlayerList();
    Block *currentBlock    = Block::ById(level->GetBlockId(pos));
    Block *combined        = currentBlock == placeBlock ? placeBlock->GetCombinedBlock(level->GetBlockData(pos), data) : nullptr;
    if (combined)
    {
        // Blocks that merge into themselves keep the merged state, others become a fresh block
        uint16_t combinedData = combined == placeBlock ? data : 0;
        level->SetBlock(pos, combined, face, combinedData);
        ConsumeHeldItem(heldItem);
        playerList->SendToLevel(m_player->GetLevel(), std::shared_ptr<Packet>(new BlockUpdateS2CPacket(
                                                              pos, (uint16_t) BlockRegistry::GetInstance()->IdOf(combined), combinedData)));
        return;
    }

    bool targetIsAir           = !currentBlock || currentBlock == air;
    bool targetIsReplaceable   = currentBlock && currentBlock != placeBlock && currentBlock->CanBeReplaced(level->GetBlockData(pos));
    bool replacingClickedBlock = targetIsReplaceable && currentBlock->IsSelectable();
    if (!targetIsAir && !targetIsReplaceable)
    {
        RejectPlace(level, pos);
        return;
    }

    if (!replacingClickedBlock)
    {
        BlockPos against(pos.x - face->dx, pos.y - face->dy, pos.z - face->dz);
        Block *againstBlock = IsChunkLoaded(level, against) ? Block::ById(level->GetBlockId(against)) : nullptr;
        if (!againstBlock || againstBlock == air || (!againstBlock->IsSelectable() && !dynamic_cast<LilyPadBlock *>(placeBlock)))
        {
            RejectPlace(level, pos);
            return;
        }
    }

    placeBlock      = placeBlock->GetPlacementBlock(face);
    BedBlock *bed   = dynamic_cast<BedBlock *>(placeBlock);
    CropBlock *crop = dynamic_cast<CropBlock *>(placeBlock);
    StemBlock *stem = dynamic_cast<StemBlock *>(placeBlock);
    if (!placeBlock || !placeBlock->CanSurvive(level, pos, placeBlock->GetAttachment(placeBlock->GetPlacementData(face))) ||
        (placeBlock->IsCollidable() && IntersectsAnyPlayer(pos, placeBlock, data)) || (bed && !bed->CanPlaceAt(level, pos, data)) ||
        (crop && !crop->CanPlaceAt(level, pos)) || (stem && !stem->CanPlaceAt(level, pos)))
    {
        RejectPlace(level, pos);
        return;
    }

    level->SetBlock(pos, placeBlock, face, data);
    playerList->SendToLevel(m_player->GetLevel(),
                            std::shared_ptr<Packet>(new BlockUpdateS2CPacket(pos, (uint16_t) level->GetBlockId(pos), level->GetBlockData(pos))));
    CompoundTag *heldTag     = heldItem->GetTag();
    BlockEntity *blockEntity = level->GetBlockEntity(pos);
    if (heldTag && heldTag->Contains("BlockEntityTag", Tag::eTagType_Compound) && blockEntity)
    {
        blockEntity->Load(heldTag->GetCompound("BlockEntityTag"));
        ((ServerLevel *) level)->MarkBlockChunkDirty(pos);
        MobSpawnerBlockEntity *spawner = dynamic_cast<MobSpawnerBlockEntity *>(blockEntity);
        if (spawner)
        {
            playerList->SendToLevel(m_player->GetLevel(), std::shared_ptr<Packet>(new SpawnerDataS2CPacket(pos, spawner->GetEntityType())));
        }
        else if (dynamic_cast<SignBlockEntity *>(blockEntity))
        {
            std::unique_ptr<CompoundTag> signTag(new CompoundTag());
            blockEntity->Save(signTag.get());
            playerList->SendToLevel(m_player->GetLevel(), std::shared_ptr<Packet>(new SignDataS2CPacket(pos, std::move(signTag))));
        }
    }

    ConsumeHeldItem(heldItem);
    if (dynamic_cast<SignBlock *>(placeBlock))
    {
        OpenSignEditor(level, pos, true);
    }

    // The placer already played this sound locally
    const std::string &placeSound = placeBlock->GetPlaceSound();
    if (!placeSound.empty())
    {
        Random random;
        std::string name = placeBlock->GetPlaceSoundVariants() > 0
                                   ? placeSound + std::to_string(random.NextInt(1, placeBlock->GetPlaceSoundVariants()))
                                   : placeSound;
        glm::vec3 center((float) pos.x + 0.5f, (float) pos.y + 0.5f, (float) pos.z + 0.5f);
        playerList->SendToOthers(m_player, std::shared_ptr<Packet>(new SoundS2CPacket(name, 1.0f, placeBlock->GetPlaceSoundPitch(), center)));
    }
}

void ServerPlayerGameMode::ResendBlock(Level *level, const BlockPos &pos)
{
    if (IsChunkLoaded(level, pos))
    {
        m_player->GetPlayerConnection()->Send(
                std::shared_ptr<Packet>(new BlockUpdateS2CPacket(pos, (uint16_t) level->GetBlockId(pos), level->GetBlockData(pos))));
    }
}

bool ServerPlayerGameMode::IsChunkLoaded(Level *level, const BlockPos &pos) const
{
    return pos.y >= 0 && pos.y < Chunk::s_sizeY &&
           level->HasChunk(ChunkPos(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ)));
}

bool ServerPlayerGameMode::CanInteract(Level *level, const BlockPos &pos) const
{
    return IsChunkLoaded(level, pos) && m_player->CanReach(pos) && level->GetWorldBorder().IsWithinBounds(pos);
}

bool ServerPlayerGameMode::IntersectsAnyPlayer(const BlockPos &pos, Block *block, uint16_t data) const
{
    std::vector<AABB> boxes;
    block->GetCollisionBoxes(data, &boxes);
    const std::vector<std::unique_ptr<ServerPlayer>> &players = m_player->GetServer()->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < boxes.size(); i++)
    {
        AABB blockBox = boxes[i].Translated(glm::dvec3((double) pos.x, (double) pos.y, (double) pos.z));
        for (size_t j = 0; j < players.size(); j++)
        {
            if (players[j]->GetLevel() == m_player->GetLevel() &&
                blockBox.Intersects(players[j]->GetBoundingBox().Translated(players[j]->GetPosition())))
            {
                return true;
            }
        }
    }

    return false;
}

void ServerPlayerGameMode::ConsumeHeldItem(ItemInstance *heldItem)
{
    if (!GameType::HasInfiniteItems(m_player->GetGameType()))
    {
        heldItem->HurtAndBreak(1, m_player);
    }
}

void ServerPlayerGameMode::RejectPlace(Level *level, const BlockPos &pos)
{
    ResendBlock(level, pos);
    m_player->GetServer()->GetPlayerList()->SendInventoryTo(m_player);
}
