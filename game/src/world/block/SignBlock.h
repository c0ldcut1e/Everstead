#pragma once

#include "Block.h"

class SignBlock : public Block
{
public:
    SignBlock(const std::string &planksPath, const std::string &entityTexturePath, SignBlock *standingBlock);

    const char *GetBlockEntityId() const override;
    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    Direction *GetAttachment(uint16_t data) const override;
    bool IsUsable(uint16_t data) const override;
    bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const override;

    bool IsWall() const;
    SignBlock *GetStandingBlock();
    const std::string &GetEntityTexturePath() const;
    float GetYRot(uint16_t data) const;

private:
    std::string m_entityTexturePath;
    SignBlock *m_standingBlock;
    int m_rotationProperty;
};
