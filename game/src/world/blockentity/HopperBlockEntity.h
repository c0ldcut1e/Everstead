#pragma once

#include <cstdint>

#include "../../utils/AABB.h"
#include "ItemTransferBlockEntity.h"

class Direction;

class HopperBlockEntity : public ItemTransferBlockEntity
{
public:
    static const int s_slotCount = 5;

    HopperBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;

    static bool SuckInItems(Level *level, AbstractContainer *hopper, const BlockPos &sourcePos, const AABB &captureBox, HopperBlockEntity *self,
                            std::unique_ptr<CompoundContainer> *compoundChest);

protected:
    void OnFirstItemReceived(int64_t sourceTickedGameTime) override;

private:
    bool PushItems(Direction *facing);
    static bool CanExtract(AbstractContainer *container, int slot, const ItemInstance &item);

    int m_cooldown;
};
