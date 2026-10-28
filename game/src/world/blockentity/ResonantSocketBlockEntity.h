#pragma once

#include "../inventory/ResonantSocketContainer.h"
#include "BlockEntity.h"

class ResonantSocketBlockEntity : public BlockEntity, public ResonantSocketContainer
{
public:
    ResonantSocketBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;
};
