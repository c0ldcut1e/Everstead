#pragma once

#include "../inventory/FurnaceContainer.h"
#include "BlockEntity.h"

class FurnaceBlockEntity : public BlockEntity, public FurnaceContainer
{
public:
    FurnaceBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;

    bool IsSwappingBlock() const;

private:
    bool m_swappingBlock;
};
