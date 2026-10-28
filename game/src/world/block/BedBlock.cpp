#include "BedBlock.h"

#include <cmath>
#include <utility>

#include "../Level.h"

static Direction *const s_bedHeadDirections[] = {Direction::south, Direction::west, Direction::north, Direction::east};

BedBlock::BedBlock(const std::string &color) : Block(false, "assets/texture/block/" + color + "_bed_feet_top.png")
{
    SetBlocksLiquid(true);
    m_facingProperty    = AddProperty("facing", 4);
    m_headProperty      = AddProperty("head", 2);
    m_occupiedProperty  = AddProperty("occupied", 2);
    m_headTop           = AddTextureVariant("assets/texture/block/" + color + "_bed_head_top.png");
    m_footTop           = AddTextureVariant("assets/texture/block/" + color + "_bed_feet_top.png");
    m_headTopTransposed = AddTextureVariant("assets/texture/block/" + color + "_bed_head_top_transposed.png");
    m_footTopTransposed = AddTextureVariant("assets/texture/block/" + color + "_bed_feet_top_transposed.png");
    m_headEnd           = AddTextureVariant("assets/texture/block/" + color + "_bed_head_end.png");
    m_footEnd           = AddTextureVariant("assets/texture/block/" + color + "_bed_feet_end.png");
    m_headSide          = AddTextureVariant("assets/texture/block/" + color + "_bed_head_side.png");
    m_footSide          = AddTextureVariant("assets/texture/block/" + color + "_bed_feet_side.png");
    m_bottom            = AddTextureVariant("assets/texture/block/planks_oak.png");
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetBreakSound("dig.wood");
}

int BedBlock::GetFacing(uint16_t data) const
{
    return GetPropertyValue(data, m_facingProperty);
}

bool BedBlock::IsHead(uint16_t data) const
{
    return GetPropertyValue(data, m_headProperty) != 0;
}

bool BedBlock::IsOccupied(uint16_t data) const
{
    return GetPropertyValue(data, m_occupiedProperty) != 0;
}

Direction *BedBlock::GetHeadDirection(uint16_t data) const
{
    return s_bedHeadDirections[GetFacing(data)];
}

BlockPos BedBlock::GetHeadPos(const BlockPos &pos, uint16_t data) const
{
    if (IsHead(data))
    {
        return pos;
    }

    Direction *head = GetHeadDirection(data);
    return BlockPos(pos.x + head->dx, pos.y, pos.z + head->dz);
}

BlockPos BedBlock::GetOtherPos(const BlockPos &pos, uint16_t data) const
{
    Direction *head = GetHeadDirection(data);
    int sign        = IsHead(data) ? -1 : 1;
    return BlockPos(pos.x + head->dx * sign, pos.y, pos.z + head->dz * sign);
}

void BedBlock::SetOccupied(Level *level, const BlockPos &pos, bool occupied)
{
    uint16_t data     = level->GetBlockData(pos);
    BlockPos parts[2] = {pos, GetOtherPos(pos, data)};
    for (int i = 0; i < 2; i++)
    {
        if (!IsBedAt(level, parts[i]))
        {
            continue;
        }

        level->SetBlockData(parts[i], WithPropertyValue(level->GetBlockData(parts[i]), m_occupiedProperty, occupied ? 1 : 0));
        level->RecordBlockChange(parts[i]);
    }
}

bool BedBlock::IsBedAt(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(pos)) == this;
}

bool BedBlock::IsFreeWithSupport(Level *level, const BlockPos &pos) const
{
    BlockPos below(pos.x, pos.y - 1, pos.z);
    Block *block      = Block::ById(level->GetBlockId(pos));
    Block *belowBlock = Block::ById(level->GetBlockId(below));
    bool free         = block == Block::ByName("air") || block->CanBeReplaced(level->GetBlockData(pos));
    return free && belowBlock->IsFaceSturdy(level->GetBlockData(below), Direction::up);
}

bool BedBlock::FindStandUpPosition(Level *level, const BlockPos &pos, BlockPos *out) const
{
    uint16_t data   = level->GetBlockData(pos);
    BlockPos head   = GetHeadPos(pos, data);
    Direction *step = GetHeadDirection(data);
    for (int i = 0; i <= 1; i++)
    {
        int startX = head.x - step->dx * i - 1;
        int startZ = head.z - step->dz * i - 1;
        for (int x = startX; x <= startX + 2; x++)
        {
            for (int z = startZ; z <= startZ + 2; z++)
            {
                BlockPos candidate(x, head.y, z);
                if (IsFreeWithSupport(level, candidate) && !Block::ById(level->GetBlockId(BlockPos(x, head.y + 1, z)))->IsSolid())
                {
                    *out = candidate;
                    return true;
                }
            }
        }
    }

    return false;
}

bool BedBlock::CanPlaceAt(Level *level, const BlockPos &pos, uint16_t data) const
{
    return !IsHead(data) && !IsOccupied(data) && IsFreeWithSupport(level, pos) && IsFreeWithSupport(level, GetOtherPos(pos, data));
}

void BedBlock::OnPlace(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    if (IsHead(data))
    {
        return;
    }

    BlockPos head = GetOtherPos(pos, data);
    level->SetBlock(head, this, nullptr, WithPropertyValue(data, m_headProperty, 1));
    level->RecordBlockChange(head);
}

void BedBlock::OnClientPlace(Level *level, const BlockPos &pos)
{
    OnPlace(level, pos);
}

void BedBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 9.0 / 16.0, 1.0)));
}

void BedBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, 9.0 / 16.0, 1.0)));
    out->push_back(AABB(glm::dvec3(0.0, 3.0 / 16.0, 0.0), glm::dvec3(1.0, 3.0 / 16.0, 1.0)));
}

bool BedBlock::HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const
{
    // The underside sits above the legs, so the leg gap in the side textures doesn't show into an empty bed
    if (boxIndex == 1 || direction == Direction::down)
    {
        return boxIndex == 1 && direction == Direction::down;
    }

    Direction *head = GetHeadDirection(data);
    return direction != (IsHead(data) ? Direction::Opposite(head) : head);
}

bool BedBlock::IsBoxFaceDoubleSided(uint16_t data, size_t boxIndex, Direction *direction) const
{
    (void) data;

    return boxIndex == 0 && direction != Direction::up;
}

Block *BedBlock::GetPlacementBlock(Direction *face)
{
    return face == Direction::up ? this : nullptr;
}

uint16_t BedBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (std::fabs(look.x) > std::fabs(look.z))
    {
        facing = look.x > 0.0 ? 3 : 1;
    }
    else
    {
        facing = look.z > 0.0 ? 0 : 2;
    }

    return WithPropertyValue(0, m_facingProperty, facing);
}

bool BedBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    if (!IsBedAt(level, pos))
    {
        return true;
    }

    return IsBedAt(level, GetOtherPos(pos, level->GetBlockData(pos)));
}

bool BedBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

Block::UVRect BedBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;

    int facing = GetFacing(data);
    bool head  = IsHead(data);
    *flipU     = false;
    if (direction == Direction::down)
    {
        return GetTextureVariantAtlasUVRect(m_bottom);
    }

    if (direction == Direction::up)
    {
        bool transposed = facing == 0 || facing == 2;
        if (transposed)
        {
            return GetTextureVariantAtlasUVRect(head ? m_headTopTransposed : m_footTopTransposed);
        }

        return GetTextureVariantAtlasUVRect(head ? m_headTop : m_footTop);
    }

    Direction *headDirection = s_bedHeadDirections[facing];
    if (direction == headDirection || direction == Direction::Opposite(headDirection))
    {
        return GetTextureVariantAtlasUVRect(head ? m_headEnd : m_footEnd);
    }

    Direction *const flippedSides[] = {Direction::east, Direction::south, Direction::west, Direction::north};
    *flipU                          = direction == flippedSides[facing];
    return GetTextureVariantAtlasUVRect(head ? m_headSide : m_footSide);
}

void BedBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    (void) box;

    if (direction != Direction::up)
    {
        return;
    }

    int facing = GetFacing(data);
    if (facing == 1 || facing == 0)
    {
        std::swap(*v0, *v1);
    }

    if (facing == 3 || facing == 0)
    {
        std::swap(*u0, *u1);
    }
}
