#include "ServerContainerMenu.h"

#include <algorithm>
#include <cmath>

#include "../../core/EversteadServer.h"
#include "../../core/GameInfo.h"
#include "../../entity/item/ExperienceOrb.h"
#include "../../entity/item/Minecart.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../network/PlayerList.h"
#include "../../network/listener/PlayerConnection.h"
#include "../../network/packet/c2s/ContainerClickC2SPacket.h"
#include "../../network/packet/s2c/ChestOpenS2CPacket.h"
#include "../../network/packet/s2c/ContainerCloseS2CPacket.h"
#include "../../network/packet/s2c/ContainerOpenS2CPacket.h"
#include "../../network/packet/s2c/ContainerSetContentS2CPacket.h"
#include "../../network/packet/s2c/ContainerSetDataS2CPacket.h"
#include "../../utils/Direction.h"
#include "../ServerLevel.h"
#include "../block/AnvilBlock.h"
#include "../block/Block.h"
#include "../block/ChestBlock.h"
#include "../block/CraftingTableBlock.h"
#include "../blockentity/FurnaceBlockEntity.h"
#include "../blockentity/ItemTransferBlockEntity.h"
#include "../blockentity/ResonantSocketBlockEntity.h"
#include "../blockentity/ResonatorBlockEntity.h"
#include "../crafting/FurnaceRecipes.h"
#include "InventoryMenu.h"

#include <glm/gtx/norm.hpp>

ServerContainerMenu::ServerContainerMenu(ServerPlayer *player)
{
    m_player             = player;
    m_craftingTablePos   = BlockPos(0, 0, 0);
    m_anvilPos           = BlockPos(0, 0, 0);
    m_blockEntityPos     = BlockPos(0, 0, 0);
    m_itemTransferMenuId = 0;
    for (int i = 0; i < 3; i++)
    {
        m_sentFurnaceData[i] = 0;
    }

    m_sentResonatorProgress = 0;
    m_chestCount            = 0;
    m_minecartId            = -1;
    m_chestMenuId           = InventoryMenu::s_chestMenuId;
}

void ServerContainerMenu::Tick()
{
    AbstractContainer *openContainer = m_player->GetOpenContainer();
    if (openContainer == m_player->GetCraftingContainer() &&
        (!m_player->CanReach(m_craftingTablePos) ||
         !dynamic_cast<CraftingTableBlock *>(Block::ById(m_player->GetLevel()->GetBlockId(m_craftingTablePos)))))
    {
        Close();
    }
    else if (openContainer == m_player->GetAnvilContainer())
    {
        if (!m_player->CanReach(m_anvilPos) || !dynamic_cast<AnvilBlock *>(Block::ById(m_player->GetLevel()->GetBlockId(m_anvilPos))))
        {
            Close();
        }
    }
    else if (m_chestCount > 0 || m_minecartId >= 0)
    {
        if (IsChestOpen())
        {
            SyncContainer(m_chestMenuId, false);
        }
        else
        {
            Close();
        }
    }
    else if (openContainer && openContainer != m_player->GetCraftingContainer())
    {
        BlockEntity *blockEntity = m_player->GetLevel()->GetBlockEntity(m_blockEntityPos);
        if (!m_player->CanReach(m_blockEntityPos) || dynamic_cast<AbstractContainer *>(blockEntity) != openContainer)
        {
            Close();
        }
        else if (dynamic_cast<FurnaceBlockEntity *>(blockEntity))
        {
            SyncFurnace(false);
        }
        else if (dynamic_cast<ResonatorBlockEntity *>(blockEntity))
        {
            SyncResonator(false);
        }
        else if (dynamic_cast<ItemTransferBlockEntity *>(blockEntity))
        {
            SyncContainer(m_itemTransferMenuId, false);
        }
        else
        {
            SyncResonantSocket(false);
        }
    }
}

void ServerContainerMenu::OpenCraftingTable(const BlockPos &pos)
{
    m_craftingTablePos = pos;
    m_player->OpenContainer(m_player->GetCraftingContainer());
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(InventoryMenu::s_craftingMenuId, 0)));
}

void ServerContainerMenu::OpenAnvil(const BlockPos &pos)
{
    m_anvilPos = pos;
    m_player->OpenContainer(m_player->GetAnvilContainer());
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(InventoryMenu::s_anvilMenuId, 0)));
    SyncAnvil();
}

void ServerContainerMenu::RenameAnvilItem(const std::string &name)
{
    AnvilContainer *anvil = m_player->GetAnvilContainer();
    if (m_player->GetOpenContainer() != anvil)
    {
        return;
    }

    anvil->SetItemName(name);
    anvil->UpdateResult();
    SyncAnvil();
}

void ServerContainerMenu::OpenFurnace(Level *level, const BlockPos &pos)
{
    m_blockEntityPos = pos;
    m_player->OpenContainer((FurnaceBlockEntity *) level->GetBlockEntity(pos));
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(InventoryMenu::s_furnaceMenuId, 0)));
    SyncFurnace(true);
}

void ServerContainerMenu::OpenResonantSocket(Level *level, const BlockPos &pos)
{
    m_blockEntityPos = pos;
    m_player->OpenContainer((ResonantSocketBlockEntity *) level->GetBlockEntity(pos));
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(InventoryMenu::s_resonantSocketMenuId, 0)));
    SyncResonantSocket(true);
}

void ServerContainerMenu::OpenResonator(Level *level, const BlockPos &pos)
{
    m_blockEntityPos = pos;
    m_player->OpenContainer((ResonatorBlockEntity *) level->GetBlockEntity(pos));
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(InventoryMenu::s_resonatorMenuId, 0)));
    SyncResonator(true);
}

void ServerContainerMenu::OpenChest(Level *level, const BlockPos &pos, ChestBlock *block)
{
    Direction *partner = block->GetPartner(level->GetBlockData(pos));
    BlockPos partnerPos(pos.x + (partner ? partner->dx : 0), pos.y, pos.z + (partner ? partner->dz : 0));
    if (block->IsBlocked(level, pos) || (partner && block->IsBlocked(level, partnerPos)))
    {
        return;
    }

    ServerLevel *serverLevel = (ServerLevel *) level;
    m_chestCount             = partner ? 2 : 1;
    m_chestPos[0]            = pos;
    m_chestPos[1]            = partnerPos;
    if (partner == Direction::west || partner == Direction::north)
    {
        m_chestPos[0] = partnerPos;
        m_chestPos[1] = pos;
    }

    for (int i = 0; i < m_chestCount; i++)
    {
        m_chests[i] = (ChestBlockEntity *) serverLevel->GetBlockEntity(m_chestPos[i]);
        m_chests[i]->StartOpen();
        serverLevel->SendToNearby(glm::dvec3(m_chestPos[i].x + 0.5, m_chestPos[i].y + 0.5, m_chestPos[i].z + 0.5), 64.0,
                                  std::shared_ptr<Packet>(new ChestOpenS2CPacket(m_chestPos[i], (uint8_t) m_chests[i]->GetOpenCount())));
    }

    AbstractContainer *container = m_chests[0];
    if (partner)
    {
        m_compoundChest.reset(new CompoundContainer(m_chests[0], m_chests[1]));
        container = m_compoundChest.get();
    }

    m_player->OpenContainer(container);
    m_player->GetPlayerConnection()->Send(
            std::shared_ptr<Packet>(new ContainerOpenS2CPacket(InventoryMenu::s_chestMenuId, (uint8_t) container->GetRows())));
    SyncContainer(InventoryMenu::s_chestMenuId, true);
}

void ServerContainerMenu::OpenMinecart(Minecart *minecart, uint8_t menuId)
{
    m_minecartId  = minecart->GetId();
    m_chestMenuId = menuId;
    m_player->OpenContainer(minecart->GetContainer());
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(menuId, (uint8_t) minecart->GetContainer()->GetRows())));
    SyncContainer(menuId, true);
}

void ServerContainerMenu::OpenItemTransfer(Level *level, const BlockPos &pos, uint8_t menuId)
{
    ItemTransferBlockEntity *blockEntity = (ItemTransferBlockEntity *) level->GetBlockEntity(pos);
    m_blockEntityPos                     = pos;
    m_itemTransferMenuId                 = menuId;
    m_player->OpenContainer(blockEntity);
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerOpenS2CPacket(menuId, (uint8_t) blockEntity->GetRows())));
    SyncContainer(menuId, true);
}

void ServerContainerMenu::Click(ContainerClickC2SPacket *packet)
{
    PlayerList *playerList       = m_player->GetServer()->GetPlayerList();
    uint8_t menuId               = packet->GetMenuId();
    bool crafting                = menuId == InventoryMenu::s_craftingMenuId;
    bool furnace                 = menuId == InventoryMenu::s_furnaceMenuId;
    bool chest                   = menuId == m_chestMenuId;
    bool resonantSocket          = menuId == InventoryMenu::s_resonantSocketMenuId;
    bool resonator               = menuId == InventoryMenu::s_resonatorMenuId;
    bool anvil                   = menuId == InventoryMenu::s_anvilMenuId;
    bool itemTransfer            = menuId == m_itemTransferMenuId && menuId != 0;
    AbstractContainer *container = crafting ? m_player->GetCraftingContainer() : nullptr;
    if (anvil)
    {
        container = m_player->GetAnvilContainer();
    }
    else if (menuId == InventoryMenu::s_playerMenuId || menuId == InventoryMenu::s_creativeMenuId)
    {
        container = m_player->GetInventoryCraftingContainer();
    }
    else if (furnace)
    {
        container = dynamic_cast<FurnaceBlockEntity *>(m_player->GetLevel()->GetBlockEntity(m_blockEntityPos));
    }
    else if (resonantSocket)
    {
        container = dynamic_cast<ResonantSocketBlockEntity *>(m_player->GetLevel()->GetBlockEntity(m_blockEntityPos));
    }
    else if (resonator)
    {
        container = dynamic_cast<ResonatorBlockEntity *>(m_player->GetLevel()->GetBlockEntity(m_blockEntityPos));
    }
    else if (itemTransfer)
    {
        container = dynamic_cast<ItemTransferBlockEntity *>(m_player->GetLevel()->GetBlockEntity(m_blockEntityPos));
    }
    else if (chest && (m_chestCount > 0 || m_minecartId >= 0) && IsChestOpen())
    {
        container = m_player->GetOpenContainer();
    }

    eGAME_TYPE gameType = m_player->GetGameType();
    if ((menuId != InventoryMenu::s_playerMenuId && menuId != InventoryMenu::s_creativeMenuId && !crafting && !furnace && !chest && !resonantSocket &&
         !resonator && !itemTransfer && !anvil) ||
        ((crafting || furnace || chest || resonantSocket || resonator || itemTransfer || anvil) &&
         (!container || m_player->GetOpenContainer() != container)) ||
        ((menuId == InventoryMenu::s_creativeMenuId || packet->GetSlot() == InventoryMenu::s_deleteSlot || packet->GetType() == eClickType_Clone) &&
         !GameType::HasInfiniteItems(gameType)) ||
        GameType::IsSpectator(gameType) || m_player->GetHealth() <= 0)
    {
        playerList->SendInventoryTo(m_player);
        return;
    }

    Inventory *inventory = m_player->GetInventory();
    Item *oldArmor[Inventory::s_armorSize];
    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        oldArmor[i] = inventory->GetArmor(i)->IsEmpty() ? nullptr : inventory->GetArmor(i)->GetItem();
    }

    int anvilCost          = anvil ? m_player->GetAnvilContainer()->GetCost() : 0;
    ItemInstance oldResult = furnace ? *container->GetItem(FurnaceContainer::s_resultSlot) : ItemInstance();
    InventoryMenu menu(inventory, container, menuId);
    if (packet->GetType() == eClickType_Drag)
    {
        menu.Drag(packet->GetDragSlots(), packet->GetButton());
    }
    else if (packet->GetType() < eClickType_Count)
    {
        menu.Click(packet->GetSlot(), packet->GetButton(), packet->GetType());
    }

    std::vector<ItemInstance> thrown;
    menu.TakeThrown(&thrown);
    for (size_t i = 0; i < thrown.size(); i++)
    {
        if (kStickFight)
        {
            m_player->GetInventory()->AddItem(thrown[i]);
        }
        else
        {
            m_player->ThrowItem(thrown[i]);
        }
    }

    ServerLevel *level = (ServerLevel *) m_player->GetLevel();
    if (furnace)
    {
        level->MarkBlockChunkDirty(m_blockEntityPos);

        const ItemInstance *result = container->GetItem(FurnaceContainer::s_resultSlot);
        int taken                  = oldResult.GetCount() - (result->IsSameItem(oldResult) ? result->GetCount() : 0);
        float experience           = taken > 0 ? (float) taken * FurnaceRecipes::GetInstance()->GetExperience(oldResult.GetItem()) : 0.0f;
        int amount                 = (int) floorf(experience);
        if ((float) amount < experience && m_random.NextFloat() < experience - (float) amount)
        {
            amount++;
        }

        const glm::dvec3 &position = m_player->GetPosition();
        ExperienceOrb::SpawnOrbs(level, glm::dvec3(position.x, position.y + 0.5, position.z + 0.5), amount);
    }

    if (resonantSocket || resonator || itemTransfer)
    {
        level->MarkBlockChunkDirty(m_blockEntityPos);
    }

    if (anvil && m_player->GetAnvilContainer()->PopTaken())
    {
        bool creative = gameType == eGameType_Creative;
        if (!creative)
        {
            m_player->GiveExperienceLevels(-anvilCost);
        }

        glm::dvec3 center(m_anvilPos.x + 0.5, m_anvilPos.y + 0.5, m_anvilPos.z + 0.5);
        AnvilBlock *anvilBlock = (AnvilBlock *) Block::ById(level->GetBlockId(m_anvilPos));
        if (!creative && m_random.NextFloat() < 0.12f)
        {
            Block *damaged = anvilBlock->GetDamaged();
            if (damaged)
            {
                level->SetBlock(m_anvilPos, damaged, nullptr, level->GetBlockData(m_anvilPos));
                level->PlaySound("random.anvil_use", center, 1.0f, m_random.NextFloat() * 0.1f + 0.9f);
            }
            else
            {
                level->SetBlock(m_anvilPos, Block::ByName("air"));
                level->PlaySound("random.anvil_break", center, 1.0f, m_random.NextFloat() * 0.1f + 0.9f);
            }

            level->RecordBlockChange(m_anvilPos);
        }
        else
        {
            level->PlaySound("random.anvil_use", center, 1.0f, m_random.NextFloat() * 0.1f + 0.9f);
        }
    }

    if (chest)
    {
        for (int i = 0; i < m_chestCount; i++)
        {
            level->MarkBlockChunkDirty(m_chestPos[i]);
        }
    }

    if ((crafting || furnace || chest || resonantSocket || resonator || itemTransfer || anvil) && packet->GetType() == eClickType_Close)
    {
        if (chest)
        {
            ReleaseChest();
        }

        m_player->CloseContainer();
    }

    playerList->SendInventoryTo(m_player);
    if (furnace && m_player->GetOpenContainer())
    {
        SyncFurnace(true);
    }
    else if (chest && m_player->GetOpenContainer())
    {
        SyncContainer(m_chestMenuId, true);
    }
    else if (itemTransfer && m_player->GetOpenContainer())
    {
        SyncContainer(m_itemTransferMenuId, true);
    }
    else if (resonantSocket && m_player->GetOpenContainer())
    {
        SyncResonantSocket(true);
    }
    else if (resonator && m_player->GetOpenContainer())
    {
        SyncResonator(true);
    }
    else if (anvil && m_player->GetOpenContainer())
    {
        SyncAnvil();
    }
    else if (crafting && m_player->GetOpenContainer())
    {
        std::vector<ItemInstance> items;
        for (int i = 0; i < container->GetSize(); i++)
        {
            items.push_back(*container->GetItem(i));
        }

        m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(
                new ContainerSetContentS2CPacket(menuId, (uint8_t) inventory->GetSelectedSlot(), items, inventory->GetCarried())));
    }

    for (int i = 0; i < Inventory::s_armorSize; i++)
    {
        if (oldArmor[i] != (inventory->GetArmor(i)->IsEmpty() ? nullptr : inventory->GetArmor(i)->GetItem()))
        {
            m_player->SendArmor(i);
        }
    }
}

void ServerContainerMenu::CloseAll()
{
    if (m_player->GetOpenContainer())
    {
        Close();
    }

    CloseMenu(m_player->GetInventoryCraftingContainer(), InventoryMenu::s_playerMenuId);
}

void ServerContainerMenu::Close()
{
    uint8_t menuId = InventoryMenu::s_furnaceMenuId;
    if (m_player->GetOpenContainer() == m_player->GetCraftingContainer())
    {
        menuId = InventoryMenu::s_craftingMenuId;
        CloseMenu(m_player->GetCraftingContainer(), menuId);
    }
    else if (m_player->GetOpenContainer() == m_player->GetAnvilContainer())
    {
        menuId = InventoryMenu::s_anvilMenuId;
        CloseMenu(m_player->GetAnvilContainer(), menuId);
    }
    else if (m_chestCount > 0 || m_minecartId >= 0)
    {
        menuId = m_chestMenuId;
        CloseMenu(nullptr, menuId);
        ReleaseChest();
    }
    else
    {
        if (dynamic_cast<ResonantSocketBlockEntity *>(m_player->GetOpenContainer()))
        {
            menuId = InventoryMenu::s_resonantSocketMenuId;
        }
        else if (dynamic_cast<ResonatorBlockEntity *>(m_player->GetOpenContainer()))
        {
            menuId = InventoryMenu::s_resonatorMenuId;
        }
        else if (dynamic_cast<ItemTransferBlockEntity *>(m_player->GetOpenContainer()))
        {
            menuId = m_itemTransferMenuId;
        }

        CloseMenu(nullptr, menuId);
    }

    m_player->CloseContainer();
    m_player->GetServer()->GetPlayerList()->SendInventoryTo(m_player);
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerCloseS2CPacket(menuId)));
}

void ServerContainerMenu::ReleaseChest()
{
    ServerLevel *level = (ServerLevel *) m_player->GetLevel();
    for (int i = 0; i < m_chestCount; i++)
    {
        if (level->GetBlockEntity(m_chestPos[i]) == m_chests[i])
        {
            m_chests[i]->StopOpen();
            level->SendToNearby(glm::dvec3(m_chestPos[i].x + 0.5, m_chestPos[i].y + 0.5, m_chestPos[i].z + 0.5), 64.0,
                                std::shared_ptr<Packet>(new ChestOpenS2CPacket(m_chestPos[i], (uint8_t) m_chests[i]->GetOpenCount())));
        }
    }

    m_chestCount  = 0;
    m_minecartId  = -1;
    m_chestMenuId = InventoryMenu::s_chestMenuId;
    m_compoundChest.reset();
    m_sentContainerItems.clear();
}

void ServerContainerMenu::SyncFurnace(bool force)
{
    FurnaceContainer *furnace = (FurnaceContainer *) m_player->GetOpenContainer();
    bool itemsChanged         = force;
    for (int i = 0; i < FurnaceContainer::s_slotCount; i++)
    {
        const ItemInstance *item = furnace->GetItem(i);
        const ItemInstance &sent = m_sentFurnaceItems[i];
        if (item->IsEmpty() != sent.IsEmpty() || (!item->IsEmpty() && (!item->IsSameItem(sent) || item->GetCount() != sent.GetCount())))
        {
            itemsChanged = true;
        }

        m_sentFurnaceItems[i] = *item;
    }

    PlayerConnection *connection = m_player->GetPlayerConnection();
    if (itemsChanged)
    {
        Inventory *inventory = m_player->GetInventory();
        std::vector<ItemInstance> items(m_sentFurnaceItems, m_sentFurnaceItems + FurnaceContainer::s_slotCount);
        connection->Send(std::shared_ptr<Packet>(new ContainerSetContentS2CPacket(
                InventoryMenu::s_furnaceMenuId, (uint8_t) inventory->GetSelectedSlot(), items, inventory->GetCarried())));
    }

    int data[3] = {furnace->GetCookTime(), furnace->GetLitTime(), furnace->GetLitDuration()};
    for (int i = 0; i < 3; i++)
    {
        if (force || data[i] != m_sentFurnaceData[i])
        {
            m_sentFurnaceData[i] = data[i];
            connection->Send(std::shared_ptr<Packet>(new ContainerSetDataS2CPacket(InventoryMenu::s_furnaceMenuId, (uint8_t) i, (int16_t) data[i])));
        }
    }
}

void ServerContainerMenu::SyncResonantSocket(bool force)
{
    const ItemInstance *item = m_player->GetOpenContainer()->GetItem(0);
    if (!force && item->IsEmpty() == m_sentResonantSocketItem.IsEmpty() &&
        (item->IsEmpty() || (item->IsSameItem(m_sentResonantSocketItem) && item->GetCount() == m_sentResonantSocketItem.GetCount())))
    {
        return;
    }

    m_sentResonantSocketItem = *item;
    Inventory *inventory     = m_player->GetInventory();
    m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ContainerSetContentS2CPacket(
            InventoryMenu::s_resonantSocketMenuId, (uint8_t) inventory->GetSelectedSlot(), {*item}, inventory->GetCarried())));
}

void ServerContainerMenu::SyncResonator(bool force)
{
    ResonatorContainer *resonator = (ResonatorContainer *) m_player->GetOpenContainer();
    bool itemsChanged             = force;
    for (int i = 0; i < ResonatorContainer::s_slotCount; i++)
    {
        const ItemInstance *item = resonator->GetItem(i);
        const ItemInstance &sent = m_sentResonatorItems[i];
        if (item->IsEmpty() != sent.IsEmpty() || (!item->IsEmpty() && (!item->IsSameItem(sent) || item->GetCount() != sent.GetCount())))
        {
            itemsChanged = true;
        }

        m_sentResonatorItems[i] = *item;
    }

    PlayerConnection *connection = m_player->GetPlayerConnection();
    if (itemsChanged)
    {
        Inventory *inventory = m_player->GetInventory();
        std::vector<ItemInstance> items(m_sentResonatorItems, m_sentResonatorItems + ResonatorContainer::s_slotCount);
        connection->Send(std::shared_ptr<Packet>(new ContainerSetContentS2CPacket(
                InventoryMenu::s_resonatorMenuId, (uint8_t) inventory->GetSelectedSlot(), items, inventory->GetCarried())));
    }

    if (force || resonator->GetProgress() != m_sentResonatorProgress)
    {
        m_sentResonatorProgress = resonator->GetProgress();
        connection->Send(
                std::shared_ptr<Packet>(new ContainerSetDataS2CPacket(InventoryMenu::s_resonatorMenuId, 0, (int16_t) m_sentResonatorProgress)));
    }
}

void ServerContainerMenu::SyncAnvil()
{
    AnvilContainer *anvil = m_player->GetAnvilContainer();
    Inventory *inventory  = m_player->GetInventory();
    std::vector<ItemInstance> items;
    for (int i = 0; i < AnvilContainer::s_slotCount; i++)
    {
        items.push_back(*anvil->GetItem(i));
    }

    PlayerConnection *connection = m_player->GetPlayerConnection();
    connection->Send(std::shared_ptr<Packet>(
            new ContainerSetContentS2CPacket(InventoryMenu::s_anvilMenuId, (uint8_t) inventory->GetSelectedSlot(), items, inventory->GetCarried())));
    connection->Send(std::shared_ptr<Packet>(
            new ContainerSetDataS2CPacket(InventoryMenu::s_anvilMenuId, 0, (int16_t) std::min(anvil->GetCost(), (int) INT16_MAX))));
}

void ServerContainerMenu::SyncContainer(uint8_t menuId, bool force)
{
    AbstractContainer *container = m_player->GetOpenContainer();
    bool itemsChanged            = force || (int) m_sentContainerItems.size() != container->GetSize();
    m_sentContainerItems.resize((size_t) container->GetSize());
    for (int i = 0; i < container->GetSize(); i++)
    {
        const ItemInstance *item = container->GetItem(i);
        const ItemInstance &sent = m_sentContainerItems[(size_t) i];
        if (item->IsEmpty() != sent.IsEmpty() || (!item->IsEmpty() && (!item->IsSameItem(sent) || item->GetCount() != sent.GetCount())))
        {
            itemsChanged = true;
        }

        m_sentContainerItems[(size_t) i] = *item;
    }

    if (itemsChanged)
    {
        Inventory *inventory = m_player->GetInventory();
        m_player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(
                new ContainerSetContentS2CPacket(menuId, (uint8_t) inventory->GetSelectedSlot(), m_sentContainerItems, inventory->GetCarried())));
    }
}

// Clicks arrive before the menu ticks, so a chest broken earlier in the same tick is already freed
bool ServerContainerMenu::IsChestOpen()
{
    if (m_minecartId >= 0)
    {
        Minecart *minecart = dynamic_cast<Minecart *>(m_player->GetLevel()->GetEntityById(m_minecartId));
        return minecart && minecart->GetContainer() == m_player->GetOpenContainer() &&
               glm::distance2(m_player->GetPosition(), minecart->GetPosition()) < 8.0 * 8.0;
    }

    bool valid = m_player->CanReach(m_chestPos[0]);
    for (int i = 0; i < m_chestCount; i++)
    {
        valid = valid && m_player->GetLevel()->GetBlockEntity(m_chestPos[i]) == m_chests[i];
    }

    return valid;
}

void ServerContainerMenu::CloseMenu(AbstractContainer *container, uint8_t menuId)
{
    InventoryMenu menu(m_player->GetInventory(), container, menuId);
    menu.Close();
    std::vector<ItemInstance> thrown;
    menu.TakeThrown(&thrown);
    for (size_t i = 0; i < thrown.size(); i++)
    {
        if (kStickFight)
        {
            m_player->GetInventory()->AddItem(thrown[i]);
        }
        else
        {
            m_player->ThrowItem(thrown[i]);
        }
    }
}
