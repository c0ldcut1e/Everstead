#pragma once

#include "Block.h"

class ChestBlock : public Block
{
public:
    ChestBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    const char *GetBlockEntityId() const override;
    bool HasAnalogOutput() const override;
    int GetAnalogOutput(Level *level, const BlockPos &pos) const override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;
    bool IsUsable(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

    int GetFacing(uint16_t data) const;
    Direction *GetPartner(uint16_t data) const;
    Direction *FindPartnerDirection(Level *level, const BlockPos &pos) const;
    bool IsBlocked(Level *level, const BlockPos &pos) const;

private:
    static bool IsSolidAt(Level *level, int x, int y, int z);

    int FindPartner(Level *level, const BlockPos &pos) const;
    bool IsFullChest(Level *level, const BlockPos &pos) const;
    void SetFacing(Level *level, const BlockPos &pos, int facing);
    void RecalcLockDir(Level *level, const BlockPos &pos);

    int m_facingProperty;
    int m_partnerProperty;
};
