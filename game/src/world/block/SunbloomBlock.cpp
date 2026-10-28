#include "SunbloomBlock.h"

#include "../Level.h"

static Direction *const s_sunbloomFaces[] = {Direction::up, Direction::down, Direction::north, Direction::south, Direction::west, Direction::east};

SunbloomBlock::SunbloomBlock(const std::string &texturePath) : Block(false, texturePath)
{
    m_faceProperty = AddProperty("face", 6);
    SetCollidable(false);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_Horizontal);
    SetCrossFollowsAttachment(true);
    SetAABB(AABB(glm::dvec3(0.3, 0.0, 0.3), glm::dvec3(0.7, 0.6, 0.7)));
    SetNeedsAttachedSupport(true);
}

uint16_t SunbloomBlock::GetPlacementData(Direction *placedAgainst) const
{
    for (int i = 0; i < 6; i++)
    {
        if (s_sunbloomFaces[i] == placedAgainst)
        {
            return WithPropertyValue(0, m_faceProperty, i);
        }
    }

    return 0;
}

Direction *SunbloomBlock::GetAttachment(uint16_t data) const
{
    return Direction::Opposite(s_sunbloomFaces[GetPropertyValue(data, m_faceProperty)]);
}

uint16_t SunbloomBlock::GetItemData() const
{
    return 0;
}

bool SunbloomBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    Direction *side = attachment ? attachment : Direction::down;
    Block *support  = Block::ById(level->GetBlockId(BlockPos(pos.x + side->dx, pos.y + side->dy, pos.z + side->dz)));
    return support == Block::ByName("sand") || support == Block::ByName("sandstone");
}

AABB SunbloomBlock::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    Direction *attachment = GetAttachment(data);
    glm::dvec3 min        = GetAABB().GetMin();
    glm::dvec3 max        = GetAABB().GetMax();
    glm::dvec3 rotatedMin = min;
    glm::dvec3 rotatedMax = max;

    if (attachment->dy > 0)
    {
        rotatedMin = glm::dvec3(min.x, 1.0 - max.y, min.z);
        rotatedMax = glm::dvec3(max.x, 1.0 - min.y, max.z);
    }
    else if (attachment->dz < 0)
    {
        rotatedMin = glm::dvec3(min.x, min.z, min.y);
        rotatedMax = glm::dvec3(max.x, max.z, max.y);
    }
    else if (attachment->dz > 0)
    {
        rotatedMin = glm::dvec3(min.x, min.z, 1.0 - max.y);
        rotatedMax = glm::dvec3(max.x, max.z, 1.0 - min.y);
    }
    else if (attachment->dx < 0)
    {
        rotatedMin = glm::dvec3(min.y, min.x, min.z);
        rotatedMax = glm::dvec3(max.y, max.x, max.z);
    }
    else if (attachment->dx > 0)
    {
        rotatedMin = glm::dvec3(1.0 - max.y, min.x, min.z);
        rotatedMax = glm::dvec3(1.0 - min.y, max.x, max.z);
    }

    return AABB(rotatedMin, rotatedMax).Translated(glm::dvec3(pos.x, pos.y, pos.z)).Translated(GetCrossOffset(pos, data));
}
