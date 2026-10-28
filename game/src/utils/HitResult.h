#pragma once

#include "../world/block/BlockPos.h"
#include "Direction.h"

#include <glm/vec3.hpp>

class HitResult
{
public:
    enum eTYPE : uint8_t
    {
        eType_MISS   = 0,
        eType_BLOCK  = 1,
        eType_ENTITY = 2,
    };

    HitResult();
    ~HitResult();

    void SetMiss();
    void SetBlock(const glm::dvec3 &hitPos, const BlockPos &pos, Direction *face, float distance);
    void SetEntity(const glm::dvec3 &hitPos, int entityId, uint64_t type, float distance);

    eTYPE GetType() const;

    bool IsMiss() const;
    bool IsBlock() const;
    bool IsEntity() const;

    const glm::dvec3 &GetHitPos() const;
    float GetDistance() const;

    const BlockPos &GetBlockPos() const;
    Direction *GetBlockFace() const;

    int GetEntityId() const;
    uint64_t GetEntityType() const;

private:
    eTYPE m_type;

    glm::dvec3 m_hitPos;
    float m_distance;

    BlockPos m_blockPos;
    Direction *m_blockFace;

    int m_entityId;
    uint64_t m_entityType;
};
