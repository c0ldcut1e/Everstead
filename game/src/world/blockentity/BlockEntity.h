#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "../block/BlockPos.h"

class CompoundTag;
class Level;

class BlockEntity
{
public:
    BlockEntity(Level *level, const BlockPos &pos);
    virtual ~BlockEntity() = default;

    static std::unique_ptr<BlockEntity> Create(const std::string &id, Level *level, const BlockPos &pos);

    virtual const char *GetId() const = 0;
    virtual void Tick();
    virtual void Save(CompoundTag *tag) const;
    virtual void Load(const CompoundTag *tag);

    Level *GetLevel() const;
    const BlockPos &GetPos() const;
    uint64_t GetTickOrder() const;
    void SetTickOrder(uint64_t tickOrder);

protected:
    Level *m_level;
    BlockPos m_pos;
    uint64_t m_tickOrder;
};
