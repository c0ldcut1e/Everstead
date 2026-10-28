#pragma once

#include "../../utils/Random.h"
#include "../inventory/Container.h"
#include "BlockEntity.h"

class ChestBlockEntity : public BlockEntity, public Container
{
public:
    static const int s_rows = 3;

    ChestBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;

    void StartOpen();
    void StopOpen();
    int GetOpenCount() const;
    void SetOpenCount(int openCount);
    float GetOpenness(float partialTicks) const;

private:
    int m_openCount;
    float m_openness;
    float m_oldOpenness;
};
