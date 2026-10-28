#include "CocoaBlock.h"

#include <algorithm>

#include "../../utils/Random.h"
#include "../Level.h"

CocoaBlock::CocoaBlock() : Block(false, "assets/texture/block/cocoa_stage0.png")
{
    SetHorizontalFacing();
    m_ageProperty = AddProperty("age", 3);
    AddTextureVariant("assets/texture/block/cocoa_stage0.png");
    AddTextureVariant("assets/texture/block/cocoa_stage1.png");
    AddTextureVariant("assets/texture/block/cocoa_stage2.png");
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetRandomTicking(true);
    SetNeedsAttachedSupport(true);
    SetSupportBlocks({"jungle_log"});
    SetBreakSound("dig.wood");
}

int CocoaBlock::GetAge(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty);
}

uint16_t CocoaBlock::WithAge(uint16_t data, int age) const
{
    return WithPropertyValue(data, m_ageProperty, age);
}

void CocoaBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    int age        = GetAge(data);
    double width   = (4 + age * 2) / 16.0;
    double height  = (5 + age * 2) / 16.0;
    double half    = width / 2.0;
    double bottom  = 0.75 - height;
    Direction *log = GetAttachment(data);
    double inset   = 1.0 / 16.0;
    double depth   = inset + width;
    if (log == Direction::north)
    {
        out->push_back(AABB(glm::dvec3(0.5 - half, bottom, inset), glm::dvec3(0.5 + half, 0.75, depth)));
    }
    else if (log == Direction::south)
    {
        out->push_back(AABB(glm::dvec3(0.5 - half, bottom, 1.0 - depth), glm::dvec3(0.5 + half, 0.75, 1.0 - inset)));
    }
    else if (log == Direction::west)
    {
        out->push_back(AABB(glm::dvec3(inset, bottom, 0.5 - half), glm::dvec3(depth, 0.75, 0.5 + half)));
    }
    else
    {
        out->push_back(AABB(glm::dvec3(1.0 - depth, bottom, 0.5 - half), glm::dvec3(1.0 - inset, 0.75, 0.5 + half)));
    }
}

void CocoaBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    GetBoxes(data, out);
    Direction *log = GetAttachment(data);
    if (log == Direction::north)
    {
        out->push_back(AABB(glm::dvec3(0.5, 0.75, 0.0), glm::dvec3(0.5, 1.0, 0.25)));
    }
    else if (log == Direction::south)
    {
        out->push_back(AABB(glm::dvec3(0.5, 0.75, 0.75), glm::dvec3(0.5, 1.0, 1.0)));
    }
    else if (log == Direction::west)
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.75, 0.5), glm::dvec3(0.25, 1.0, 0.5)));
    }
    else
    {
        out->push_back(AABB(glm::dvec3(0.75, 0.75, 0.5), glm::dvec3(1.0, 1.0, 0.5)));
    }
}

Block::UVRect CocoaBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;
    (void) direction;

    *flipU = false;
    return GetTextureVariantAtlasUVRect(GetAge(data));
}

void CocoaBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    // The stem texture touches the log on its right edge, so we flip it when the log is on the low side
    if (box.GetMin().y >= 0.75)
    {
        if (*u0 + *u1 < 1.0f)
        {
            *u0 = 1.0f - *u0;
            *u1 = 1.0f - *u1;
        }

        return;
    }

    int age    = GetAge(data);
    int width  = 4 + age * 2;
    int height = 5 + age * 2;
    if (direction->dy != 0)
    {
        // The fully grown pod has a 7 pixel top in its texture even though the pod is 8 wide
        int top = std::min(width, 7);
        *u0     = 0.0f;
        *v0     = 0.0f;
        *u1     = top / 16.0f;
        *v1     = top / 16.0f;
    }
    else
    {
        *u0 = (15 - width) / 16.0f;
        *v0 = 4.0f / 16.0f;
        *u1 = 15.0f / 16.0f;
        *v1 = (4 + height) / 16.0f;
    }
}

void CocoaBlock::RandomTick(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    if (level->IsClientSide() || GetAge(data) >= 2 || level->GetRandom().NextInt(0, 4) != 0)
    {
        return;
    }

    level->SetBlockData(pos, WithAge(data, GetAge(data) + 1));
    level->RecordBlockChange(pos);
}

Block *CocoaBlock::GetPlacementBlock(Direction *face)
{
    return face->dy != 0 ? nullptr : this;
}
