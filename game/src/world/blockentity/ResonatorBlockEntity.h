#pragma once

#include "../inventory/ResonatorContainer.h"
#include "BlockEntity.h"

class ResonatorBlockEntity : public BlockEntity, public ResonatorContainer
{
public:
    ResonatorBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;
};
