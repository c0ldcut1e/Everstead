#pragma once

#include <unordered_map>

#include "../../utils/Random.h"
#include "Block.h"

class FractureCoreBlock : public Block
{
public:
    FractureCoreBlock();

    bool IsOpen(uint16_t data) const;
    int GetStability(Level *level, const BlockPos &pos) const;
    int GetTearStability(Level *level, const glm::dvec3 &position, BlockPos &core) const;
    glm::dvec3 GetTearCenter(const BlockPos &pos) const;
    float GetTearSeed(const BlockPos &pos) const;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    void OnPass(Level *level, const BlockPos &pos, Random &random);
    void Collapse(Level *level, const BlockPos &pos);
    void Tick(Level *level, const BlockPos &pos) override;

private:
    void SetOpen(Level *level, const BlockPos &pos, bool open);

    int m_openProperty;
    int m_passesProperty;
    std::unordered_map<BlockPos, int64_t> m_lastToggle;
    std::unordered_map<BlockPos, int> m_collapseStages;
};
