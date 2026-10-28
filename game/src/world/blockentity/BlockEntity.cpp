#include "BlockEntity.h"

#include "../../tag/CompoundTag.h"
#include "ChestBlockEntity.h"
#include "DispenserBlockEntity.h"
#include "DropperBlockEntity.h"
#include "FurnaceBlockEntity.h"
#include "HopperBlockEntity.h"
#include "MobSpawnerBlockEntity.h"
#include "PistonMovingBlockEntity.h"
#include "ResonantSocketBlockEntity.h"
#include "ResonatorBlockEntity.h"
#include "SignBlockEntity.h"

BlockEntity::BlockEntity(Level *level, const BlockPos &pos)
{
    m_level     = level;
    m_pos       = pos;
    m_tickOrder = 0;
}

std::unique_ptr<BlockEntity> BlockEntity::Create(const std::string &id, Level *level, const BlockPos &pos)
{
    if (id == "chest")
    {
        return std::unique_ptr<BlockEntity>(new ChestBlockEntity(level, pos));
    }

    if (id == "dispenser")
    {
        return std::unique_ptr<BlockEntity>(new DispenserBlockEntity(level, pos));
    }

    if (id == "dropper")
    {
        return std::unique_ptr<BlockEntity>(new DropperBlockEntity(level, pos));
    }

    if (id == "furnace")
    {
        return std::unique_ptr<BlockEntity>(new FurnaceBlockEntity(level, pos));
    }

    if (id == "hopper")
    {
        return std::unique_ptr<BlockEntity>(new HopperBlockEntity(level, pos));
    }

    if (id == "mob_spawner")
    {
        return std::unique_ptr<BlockEntity>(new MobSpawnerBlockEntity(level, pos));
    }

    if (id == "piston")
    {
        return std::unique_ptr<BlockEntity>(new PistonMovingBlockEntity(level, pos));
    }

    if (id == "resonant_socket")
    {
        return std::unique_ptr<BlockEntity>(new ResonantSocketBlockEntity(level, pos));
    }

    if (id == "resonator")
    {
        return std::unique_ptr<BlockEntity>(new ResonatorBlockEntity(level, pos));
    }

    if (id == "sign")
    {
        return std::unique_ptr<BlockEntity>(new SignBlockEntity(level, pos));
    }

    return nullptr;
}

void BlockEntity::Tick()
{}

void BlockEntity::Save(CompoundTag *tag) const
{
    tag->PutString("id", GetId());
    tag->PutInt("x", m_pos.x);
    tag->PutInt("y", m_pos.y);
    tag->PutInt("z", m_pos.z);
}

void BlockEntity::Load(const CompoundTag *)
{}

Level *BlockEntity::GetLevel() const
{
    return m_level;
}

const BlockPos &BlockEntity::GetPos() const
{
    return m_pos;
}

uint64_t BlockEntity::GetTickOrder() const
{
    return m_tickOrder;
}

void BlockEntity::SetTickOrder(uint64_t tickOrder)
{
    m_tickOrder = tickOrder;
}
