#pragma once

#include <string>

#include "Block.h"

class StemBlock : public Block
{
public:
    StemBlock(const std::string &texturePrefix, const std::string &fruitName, const std::string &attachedName);

    void RandomTick(Level *level, const BlockPos &pos) override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;
    bool CanPlaceAt(Level *level, const BlockPos &pos) const;
    UVRect GetCrossAtlasUVRect(uint16_t data) const override;
    glm::dvec3 GetCrossOffset(const BlockPos &pos, uint16_t data) const override;
    AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const override;
    bool CanBoneMeal(uint16_t data) const;
    void BoneMeal(Level *level, const BlockPos &pos);
    uint16_t WithAge(int age) const;
    Block *GetFruit() const;
    Block *GetAttachedStem() const;

    static constexpr int kMaxAge = 7;

private:
    void GrowFruit(Level *level, const BlockPos &pos) const;

    int m_ageProperty;
    std::string m_fruitName;
    std::string m_attachedName;
};
