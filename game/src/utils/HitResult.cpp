#include "HitResult.h"

HitResult::HitResult()
{
    m_type       = eType_MISS;
    m_hitPos     = {0.0, 0.0, 0.0};
    m_distance   = 0.0f;
    m_blockPos   = {};
    m_blockFace  = nullptr;
    m_entityId   = 0;
    m_entityType = 0;
}

HitResult::~HitResult()
{}

void HitResult::SetMiss()
{
    m_type       = eType_MISS;
    m_hitPos     = glm::dvec3(0.0, 0.0, 0.0);
    m_distance   = 0.0f;
    m_blockPos   = BlockPos();
    m_blockFace  = nullptr;
    m_entityId   = 0;
    m_entityType = 0;
}

void HitResult::SetBlock(const glm::dvec3 &hitPos, const BlockPos &pos, Direction *face, float distance)
{
    m_type       = eType_BLOCK;
    m_hitPos     = hitPos;
    m_distance   = distance;
    m_blockPos   = pos;
    m_blockFace  = face;
    m_entityId   = 0;
    m_entityType = 0;
}

void HitResult::SetEntity(const glm::dvec3 &hitPos, int entityId, uint64_t type, float distance)
{
    m_type       = eType_ENTITY;
    m_hitPos     = hitPos;
    m_distance   = distance;
    m_blockPos   = BlockPos();
    m_blockFace  = nullptr;
    m_entityId   = entityId;
    m_entityType = type;
}

HitResult::eTYPE HitResult::GetType() const
{
    return m_type;
}

bool HitResult::IsMiss() const
{
    return m_type == eType_MISS;
}

bool HitResult::IsBlock() const
{
    return m_type == eType_BLOCK;
}

bool HitResult::IsEntity() const
{
    return m_type == eType_ENTITY;
}

const glm::dvec3 &HitResult::GetHitPos() const
{
    return m_hitPos;
}

float HitResult::GetDistance() const
{
    return m_distance;
}

const BlockPos &HitResult::GetBlockPos() const
{
    return m_blockPos;
}

Direction *HitResult::GetBlockFace() const
{
    return m_blockFace;
}

int HitResult::GetEntityId() const
{
    return m_entityId;
}

uint64_t HitResult::GetEntityType() const
{
    return m_entityType;
}
