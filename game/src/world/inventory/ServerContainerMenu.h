#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../../utils/Random.h"
#include "../block/BlockPos.h"
#include "../blockentity/ChestBlockEntity.h"
#include "CompoundContainer.h"
#include "FurnaceContainer.h"
#include "ResonatorContainer.h"

class ChestBlock;
class ContainerClickC2SPacket;
class Level;
class Minecart;
class ServerPlayer;

class ServerContainerMenu
{
public:
    ServerContainerMenu(ServerPlayer *player);

    void Tick();
    void OpenCraftingTable(const BlockPos &pos);
    void OpenAnvil(const BlockPos &pos);
    void RenameAnvilItem(const std::string &name);
    void OpenFurnace(Level *level, const BlockPos &pos);
    void OpenResonantSocket(Level *level, const BlockPos &pos);
    void OpenResonator(Level *level, const BlockPos &pos);
    void OpenChest(Level *level, const BlockPos &pos, ChestBlock *block);
    void OpenMinecart(Minecart *minecart, uint8_t menuId);
    void OpenItemTransfer(Level *level, const BlockPos &pos, uint8_t menuId);
    void Click(ContainerClickC2SPacket *packet);
    void CloseAll();

private:
    void Close();
    void ReleaseChest();
    void SyncFurnace(bool force);
    void SyncResonantSocket(bool force);
    void SyncResonator(bool force);
    void SyncAnvil();
    void SyncContainer(uint8_t menuId, bool force);
    bool IsChestOpen();
    void CloseMenu(AbstractContainer *container, uint8_t menuId);

    ServerPlayer *m_player;
    BlockPos m_craftingTablePos;
    BlockPos m_anvilPos;
    BlockPos m_blockEntityPos;
    uint8_t m_itemTransferMenuId;
    ItemInstance m_sentFurnaceItems[FurnaceContainer::s_slotCount];
    int m_sentFurnaceData[3];
    ItemInstance m_sentResonantSocketItem;
    ItemInstance m_sentResonatorItems[ResonatorContainer::s_slotCount];
    int m_sentResonatorProgress;
    BlockPos m_chestPos[2];
    ChestBlockEntity *m_chests[2];
    int m_chestCount;
    int m_minecartId;
    uint8_t m_chestMenuId;
    std::unique_ptr<CompoundContainer> m_compoundChest;
    std::vector<ItemInstance> m_sentContainerItems;
    Random m_random;
};
