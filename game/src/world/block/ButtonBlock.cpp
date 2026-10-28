#include "ButtonBlock.h"

#include "../Level.h"

// Indexed by the face the button sticks out of
static Direction *const s_buttonFaces[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};

ButtonBlock::ButtonBlock(const std::string &texturePath, int pressTicks) : SwitchBlock(texturePath)
{
    m_faceProperty    = AddProperty("face", 6);
    m_poweredProperty = AddProperty("powered", 2);
    m_pressTicks      = pressTicks;
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsAttachedSupport(true);
}

uint16_t ButtonBlock::GetPlacementData(Direction *placedAgainst) const
{
    for (int i = 0; i < 6; i++)
    {
        if (s_buttonFaces[i] == placedAgainst)
        {
            return WithPropertyValue(0, m_faceProperty, i);
        }
    }

    return WithPropertyValue(0, m_faceProperty, 1);
}

uint16_t ButtonBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) hitPos;
    (void) look;
    return GetPlacementData(face);
}

Direction *ButtonBlock::GetAttachment(uint16_t data) const
{
    return Direction::Opposite(s_buttonFaces[GetPropertyValue(data, m_faceProperty)]);
}

uint16_t ButtonBlock::GetItemData() const
{
    return WithPropertyValue(0, m_faceProperty, 3);
}

void ButtonBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p   = 1.0 / 16.0;
    double depth     = IsPowered(data) ? p : 2.0 * p;
    Direction *face  = s_buttonFaces[GetPropertyValue(data, m_faceProperty)];
    double wideMin   = 5.0 * p;
    double wideMax   = 11.0 * p;
    double narrowMin = 6.0 * p;
    double narrowMax = 10.0 * p;

    if (face->dy != 0)
    {
        double minY = face->dy > 0 ? 0.0 : 1.0 - depth;
        out->push_back(AABB(glm::dvec3(wideMin, minY, narrowMin), glm::dvec3(wideMax, minY + depth, narrowMax)));
    }
    else if (face->dx != 0)
    {
        double minX = face->dx > 0 ? 0.0 : 1.0 - depth;
        out->push_back(AABB(glm::dvec3(minX, narrowMin, wideMin), glm::dvec3(minX + depth, narrowMax, wideMax)));
    }
    else
    {
        double minZ = face->dz > 0 ? 0.0 : 1.0 - depth;
        out->push_back(AABB(glm::dvec3(wideMin, narrowMin, minZ), glm::dvec3(wideMax, narrowMax, minZ + depth)));
    }
}

void ButtonBlock::GetItemBoxes(std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    out->push_back(AABB(glm::dvec3(5.0 * p, 6.0 * p, 6.0 * p), glm::dvec3(11.0 * p, 10.0 * p, 10.0 * p)));
}

bool ButtonBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool ButtonBlock::OnUse(Level *level, const BlockPos &pos)
{
    if (IsPowered(level->GetBlockData(pos)))
    {
        return true;
    }

    SetPowered(level, pos, true);

    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    level->PlaySound("random.click", center, 0.3f, 0.6f);
    if (!level->IsClientSide())
    {
        level->ScheduleBlockForTick(pos, (uint32_t) m_pressTicks, 0);
    }

    return true;
}

void ButtonBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!IsPowered(level->GetBlockData(pos)))
    {
        return;
    }

    SetPowered(level, pos, false);

    glm::dvec3 center((double) pos.x + 0.5, (double) pos.y + 0.5, (double) pos.z + 0.5);
    level->PlaySound("random.click", center, 0.3f, 0.5f);
}
