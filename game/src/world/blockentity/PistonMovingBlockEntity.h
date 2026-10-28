#pragma once

#include <cstdint>

#include "BlockEntity.h"

#include <glm/glm.hpp>

class Block;
class Direction;

class PistonMovingBlockEntity : public BlockEntity
{
public:
    PistonMovingBlockEntity(Level *level, const BlockPos &pos);

    const char *GetId() const override;
    void Tick() override;
    void Save(CompoundTag *tag) const override;
    void Load(const CompoundTag *tag) override;
    void Init(Block *block, uint16_t data, Direction *facing, bool extending, bool source);
    void Finish();
    Block *GetBlock() const;
    uint16_t GetData() const;
    bool IsSource() const;
    bool IsExtending() const;
    Direction *GetFacing() const;
    float GetProgress() const;
    int64_t GetLastTicked() const;
    glm::dvec3 GetOffset(float partialTicks) const;

private:
    void PlaceMovedBlock(Block *block);
    void PushEntities(double step);

    Block *m_block;
    uint16_t m_data;
    Direction *m_facing;
    bool m_extending;
    bool m_source;
    float m_progress;
    float m_progressO;
    int64_t m_lastTicked;
};
