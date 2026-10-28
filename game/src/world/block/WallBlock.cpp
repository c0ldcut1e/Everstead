#include "WallBlock.h"

#include "FenceGateBlock.h"

WallBlock::WallBlock(const std::string &texturePath) : FenceBlock(texturePath)
{
    SetBreakSound("dig.stone");
}

bool WallBlock::ConnectsTo(const Block *block) const
{
    if (!block)
    {
        return false;
    }

    if (dynamic_cast<const WallBlock *>(block) || dynamic_cast<const FenceGateBlock *>(block))
    {
        return true;
    }

    return block->GetRenderShape() == eRenderShape_CUBE && block->IsSolid() && !block->IsLiquid() && !block->IsTransparent();
}

void WallBlock::AddBoxes(uint16_t data, double postHeight, double armHeight, std::vector<AABB> *out) const
{
    out->push_back(AABB(glm::dvec3(kPostMin, 0.0, kPostMin), glm::dvec3(kPostMax, postHeight, kPostMax)));
    for (int side = 0; side < 4; side++)
    {
        if (GetPropertyValue(data, m_sideProperties[side]) == 0)
        {
            continue;
        }

        Direction *direction = s_sides[side];
        if (direction->dx != 0)
        {
            double minX = direction->dx < 0 ? 0.0 : kPostMax;
            double maxX = direction->dx < 0 ? kPostMin : 1.0;
            out->push_back(AABB(glm::dvec3(minX, 0.0, kArmMin), glm::dvec3(maxX, armHeight, kArmMax)));
        }
        else
        {
            double minZ = direction->dz < 0 ? 0.0 : kPostMax;
            double maxZ = direction->dz < 0 ? kPostMin : 1.0;
            out->push_back(AABB(glm::dvec3(kArmMin, 0.0, minZ), glm::dvec3(kArmMax, armHeight, maxZ)));
        }
    }
}

void WallBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    AddBoxes(data, 1.0, 0.8125, out);
}

void WallBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    AddBoxes(data, 1.5, 1.5, out);
}
