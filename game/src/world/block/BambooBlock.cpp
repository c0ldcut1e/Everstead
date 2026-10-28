#include "BambooBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"

BambooBlock::BambooBlock() : Block(false, "assets/texture/block/bamboo_stalk.png")
{
    SetBlocksLiquid(true);
    m_ageProperty        = AddProperty("age", 2);
    m_leavesProperty     = AddProperty("leaves", 3);
    m_stageProperty      = AddProperty("stage", 2);
    m_smallLeavesTexture = AddTextureVariant("assets/texture/block/bamboo_small_leaves.png");
    m_largeLeavesTexture = AddTextureVariant("assets/texture/block/bamboo_large_leaves.png");
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetRandomTicking(true);
    SetBreakSound("bamboo.place", 6);
    SetPlaceSound("bamboo.place", 6, 0.8f);
}

bool BambooBlock::CanPlantOn(const Block *block)
{
    return block == Block::ByName("grass_block") || block == Block::ByName("dirt") || block == Block::ByName("coarse_dirt") ||
           block == Block::ByName("podzol") || block == Block::ByName("sand") || block == Block::ByName("red_sand") ||
           block == Block::ByName("gravel");
}

int BambooBlock::GetAge(uint16_t data) const
{
    return GetPropertyValue(data, m_ageProperty);
}

uint16_t BambooBlock::GetStateData(int age, eLeaves leaves, int stage) const
{
    return WithPropertyValue(WithPropertyValue(WithPropertyValue(0, m_ageProperty, age), m_leavesProperty, leaves), m_stageProperty, stage);
}

void BambooBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    double half = GetPropertyValue(data, m_ageProperty) == 0 ? 1.0 / 16.0 : 1.5 / 16.0;
    out->push_back(AABB(glm::dvec3(0.5 - half, 0.0, 0.5 - half), glm::dvec3(0.5 + half, 1.0, 0.5 + half)));
}

void BambooBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    GetBoxes(data, out);
    if (GetPropertyValue(data, m_leavesProperty) != eLeaves_None)
    {
        out->push_back(AABB(glm::dvec3(0.5, 0.0, 0.0), glm::dvec3(0.5, 1.0, 1.0)));
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.5), glm::dvec3(1.0, 1.0, 0.5)));
    }
}

void BambooBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;

    out->push_back(AABB(glm::dvec3(6.5 / 16.0, 0.0, 6.5 / 16.0), glm::dvec3(9.5 / 16.0, 1.0, 9.5 / 16.0)));
}

Block::UVRect BambooBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    *flipU = false;
    if (box.GetMax().x - box.GetMin().x < 1.0 && box.GetMax().z - box.GetMin().z < 1.0)
    {
        return GetAtlasUVRect(direction);
    }

    bool large = GetPropertyValue(data, m_leavesProperty) == eLeaves_Large;
    return GetTextureVariantAtlasUVRect(large ? m_largeLeavesTexture : m_smallLeavesTexture);
}

void BambooBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    float width = (float) (box.GetMax().x - box.GetMin().x);
    if (width >= 1.0f || box.GetMax().z - box.GetMin().z >= 1.0)
    {
        Block::AdjustBoxFaceUV(data, box, direction, u0, v0, u1, v1);
        return;
    }

    if (direction->dy != 0)
    {
        *u0 = 13.0f / 16.0f;
        *u1 = 13.0f / 16.0f + width;
        *v0 = direction == Direction::up ? 0.0f : 4.0f / 16.0f;
        *v1 = *v0 + width;
    }
    else
    {
        *u0 = 0.0f;
        *u1 = width;
        *v0 = 0.0f;
        *v1 = 1.0f;
    }
}

bool BambooBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    return below == this || below == Block::ByName("bamboo_sapling") || CanPlantOn(below);
}

void BambooBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (!level->IsClientSide() && !CanSurvive(level, pos, nullptr))
    {
        level->ScheduleBlockForTick(pos, 1, 0);
    }
}

void BambooBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!CanSurvive(level, pos, nullptr))
    {
        level->PlayBlockBreakSound(pos, this);
        Block::OnNeighborChanged(level, pos);
    }
}

void BambooBlock::RandomTick(Level *level, const BlockPos &pos)
{
    const int maxHeight = 16;

    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (level->IsClientSide() || GetPropertyValue(level->GetBlockData(pos), m_stageProperty) != 0 || level->GetRandom().NextInt(0, 2) != 0 ||
        level->GetBlockId(above) != 0 || level->GetRawBrightness(above) < 9)
    {
        return;
    }

    int height = 1;
    while (height < maxHeight && Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - height, pos.z))) == this)
    {
        height++;
    }

    if (height < maxHeight)
    {
        Grow(level, pos, height);
    }
}

void BambooBlock::Grow(Level *level, const BlockPos &pos, int height) const
{
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    BlockPos below2Pos(pos.x, pos.y - 2, pos.z);
    uint16_t belowData = level->GetBlockData(belowPos);
    bool belowBamboo   = Block::ById(level->GetBlockId(belowPos)) == this;
    bool below2Bamboo  = Block::ById(level->GetBlockId(below2Pos)) == this;
    eLeaves leaves     = eLeaves_None;
    if (height >= 1)
    {
        if (!belowBamboo || GetPropertyValue(belowData, m_leavesProperty) == eLeaves_None)
        {
            leaves = eLeaves_Small;
        }
        else
        {
            leaves = eLeaves_Large;
            if (below2Bamboo)
            {
                level->SetBlockData(belowPos, WithPropertyValue(belowData, m_leavesProperty, eLeaves_Small));
                level->RecordBlockChange(belowPos);
                level->SetBlockData(below2Pos, WithPropertyValue(level->GetBlockData(below2Pos), m_leavesProperty, eLeaves_None));
                level->RecordBlockChange(below2Pos);
            }
        }
    }

    int age   = GetPropertyValue(level->GetBlockData(pos), m_ageProperty) == 1 || below2Bamboo ? 1 : 0;
    int stage = (height >= 11 && level->GetRandom().NextFloat() < 0.25f) || height == 15 ? 1 : 0;
    BlockPos above(pos.x, pos.y + 1, pos.z);
    level->SetBlock(above, (Block *) this, nullptr, GetStateData(age, leaves, stage));
    level->RecordBlockChange(above);
}
