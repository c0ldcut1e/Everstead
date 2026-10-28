#pragma once

#include <vector>

#include "PistonPartBlock.h"

class PistonBaseBlock : public PistonPartBlock
{
public:
    PistonBaseBlock(bool sticky);

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void TriggerEvent(Level *level, const BlockPos &pos, int id) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetItemData() const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    bool IsSticky() const;
    bool IsExtended(uint16_t data) const;
    uint16_t WithExtended(uint16_t data, bool extended) const;

protected:
    int GetEndTexture(uint16_t data, bool front) const override;

private:
    void CheckState(Level *level, const BlockPos &pos);
    bool IsPowered(Level *level, const BlockPos &pos, Direction *facing) const;
    struct PushStructure
    {
        BlockPos pistonPos;
        Direction *moveDirection;
        std::vector<BlockPos> toMove;
        std::vector<BlockPos> toDestroy;
    };

    bool ResolvePushStructure(Level *level, PushStructure *structure, const BlockPos &start) const;
    bool AddBlockLine(Level *level, PushStructure *structure, const BlockPos &origin) const;
    bool AddBranchingBlocks(Level *level, PushStructure *structure, const BlockPos &from) const;
    bool MoveBlocks(Level *level, const BlockPos &pos, Direction *facing, bool extending) const;
    bool CanPush(Block *block, uint16_t data) const;
    bool IsDestroyedByPush(Block *block) const;
    void Extend(Level *level, const BlockPos &pos, uint16_t data);
    void Retract(Level *level, const BlockPos &pos, uint16_t data, int id);

    bool m_sticky;
    int m_extendedProperty;
};
