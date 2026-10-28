#include "SignBlock.h"

#include <cmath>

#include "../Level.h"

#include <glm/common.hpp>
#include <glm/trigonometric.hpp>

SignBlock::SignBlock(const std::string &planksPath, const std::string &entityTexturePath, SignBlock *standingBlock) : Block(false, planksPath)
{
    SetBlocksLiquid(true);
    m_entityTexturePath = entityTexturePath;
    m_standingBlock     = standingBlock ? standingBlock : this;
    m_rotationProperty  = -1;
    if (standingBlock)
    {
        SetHorizontalFacing();
    }
    else
    {
        m_rotationProperty = AddProperty("rotation", 16);
    }

    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetEntityRendered(true);
    SetFlatItem(true);
    SetBreakSound("dig.wood");
}

const char *SignBlock::GetBlockEntityId() const
{
    return "sign";
}

void SignBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    if (!IsWall())
    {
        out->push_back(AABB(glm::dvec3(4.0 / 16.0, 0.0, 4.0 / 16.0), glm::dvec3(12.0 / 16.0, 1.0, 12.0 / 16.0)));
        return;
    }

    Direction *attachment = GetAttachment(data);
    glm::dvec3 min(0.0, 4.5 / 16.0, 0.0);
    glm::dvec3 max(1.0, 12.5 / 16.0, 1.0);
    if (attachment == Direction::north)
    {
        max.z = 2.0 / 16.0;
    }
    else if (attachment == Direction::south)
    {
        min.z = 14.0 / 16.0;
    }
    else if (attachment == Direction::west)
    {
        max.x = 2.0 / 16.0;
    }
    else
    {
        min.x = 14.0 / 16.0;
    }

    out->push_back(AABB(min, max));
}

void SignBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    (void) out;
}

uint16_t SignBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) hitPos;

    if (IsWall())
    {
        return GetPlacementData(face);
    }

    float yaw = (float) glm::degrees(atan2(-look.x, look.z));
    return WithPropertyValue(0, m_rotationProperty, (int) floor((yaw + 180.0f) / 22.5f + 0.5f) & 15);
}

Direction *SignBlock::GetAttachment(uint16_t data) const
{
    return IsWall() ? Block::GetAttachment(data) : Direction::down;
}

bool SignBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool SignBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    BlockPos supportPos(pos.x + attachment->dx, pos.y + attachment->dy, pos.z + attachment->dz);
    Block *support = Block::ById(level->GetBlockId(supportPos));
    if (dynamic_cast<SignBlock *>(support))
    {
        return true;
    }

    if (!support->IsCollidable() || support->IsLiquid())
    {
        return false;
    }

    if (support->GetRenderShape() == eRenderShape_CUBE)
    {
        return true;
    }

    std::vector<AABB> boxes;
    support->GetCollisionBoxes(level->GetBlockData(supportPos), &boxes);
    if (boxes.empty())
    {
        return false;
    }

    glm::dvec3 min = boxes[0].GetMin();
    glm::dvec3 max = boxes[0].GetMax();
    for (size_t i = 1; i < boxes.size(); i++)
    {
        min = glm::min(min, boxes[i].GetMin());
        max = glm::max(max, boxes[i].GetMax());
    }

    glm::dvec3 size = max - min;
    return (size.x + size.y + size.z) / 3.0 >= 0.7291666666666666 || size.y >= 1.0;
}

bool SignBlock::IsWall() const
{
    return m_rotationProperty < 0;
}

SignBlock *SignBlock::GetStandingBlock()
{
    return m_standingBlock;
}

const std::string &SignBlock::GetEntityTexturePath() const
{
    return m_entityTexturePath;
}

float SignBlock::GetYRot(uint16_t data) const
{
    if (!IsWall())
    {
        return GetPropertyValue(data, m_rotationProperty) * 22.5f;
    }

    constexpr float kFacingYRots[] = {180.0f, 0.0f, 90.0f, 270.0f};
    return kFacingYRots[GetPropertyValue(data, FindProperty("facing"))];
}
