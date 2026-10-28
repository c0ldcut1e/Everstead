#pragma once

#include <cstdint>
#include <memory>

#include "../inventory/CompoundContainer.h"
#include "../inventory/Container.h"
#include "BlockEntity.h"

class Direction;

class ItemTransferBlockEntity : public BlockEntity, public Container
{
public:
    ItemTransferBlockEntity(Level *level, const BlockPos &pos, int rows, int columns);

    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;

    static AbstractContainer *GetContainerAt(Level *level, const BlockPos &pos, BlockPos *partnerPos,
                                             std::unique_ptr<CompoundContainer> *compoundChest);
    static ItemInstance Insert(AbstractContainer *container, ItemInstance item, Direction *side, ItemTransferBlockEntity *source);
    static void MarkDirty(Level *level, const BlockPos &pos, const BlockPos &partnerPos);

protected:
    virtual void OnFirstItemReceived(int64_t sourceTickedGameTime);

    int64_t m_tickedGameTime;
    std::unique_ptr<CompoundContainer> m_compoundChest;

private:
    static bool CanInsert(AbstractContainer *container, int slot, const ItemInstance &item, Direction *side);
};
