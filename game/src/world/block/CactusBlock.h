#pragma once

#include "Block.h"

class CactusBlock : public Block
{
public:
    CactusBlock(const std::string &sidePath, const std::string &topPath, const std::string &bottomPath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const override;
    bool HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const override;
    bool IsBoxFaceDoubleSided(uint16_t data, size_t boxIndex, Direction *direction) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    void RandomTick(Level *level, const BlockPos &pos) override;

private:
    static constexpr double kInset = 1.0 / 16.0;

    int m_ageProperty;
};
