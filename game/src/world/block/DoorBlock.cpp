#include "DoorBlock.h"

#include <cmath>

#include "../Level.h"
#include "../chunk/Chunk.h"

static Direction *const s_doorSides[] = {Direction::north, Direction::east, Direction::south, Direction::west};

DoorBlock::DoorBlock(const std::string &lowerTexturePath, const std::string &upperTexturePath) : OpenableBlock(false, lowerTexturePath)
{
    SetBlocksLiquid(true);
    m_facingProperty  = AddProperty("facing", 4);
    m_halfProperty    = AddProperty("half", 2);
    m_hingeProperty   = AddProperty("hinge", 2);
    m_openProperty    = AddProperty("open", 2);
    m_poweredProperty = AddProperty("powered", 2);
    SetTransparent(true);
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetFlatItem(true);
    SetSecondaryTexture(upperTexturePath);
}

bool DoorBlock::IsUpper(uint16_t data) const
{
    return GetPropertyValue(data, m_halfProperty) != 0;
}

bool DoorBlock::IsHingeRight(uint16_t data) const
{
    return GetPropertyValue(data, m_hingeProperty) != 0;
}

int DoorBlock::GetFacing(uint16_t data) const
{
    return GetPropertyValue(data, m_facingProperty);
}

bool DoorBlock::IsDoorAt(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(pos)) == this;
}

int DoorBlock::CountSolid(Level *level, const BlockPos &pos) const
{
    int count = 0;
    for (int dy = 0; dy < 2; dy++)
    {
        Block *block = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + dy, pos.z)));
        if (block && block->IsSolid() && !block->IsTransparent() && block->GetRenderShape() == eRenderShape_CUBE)
        {
            count++;
        }
    }

    return count;
}

bool DoorBlock::HasSturdyTop(Level *level, const BlockPos &pos) const
{
    Block *block = Block::ById(level->GetBlockId(pos));
    if (!block || block->IsLiquid() || !block->IsCollidable())
    {
        return false;
    }

    if (block->GetRenderShape() == eRenderShape_CUBE)
    {
        return block->IsSolid();
    }

    if (block->GetRenderShape() != eRenderShape_BOXES)
    {
        return false;
    }

    std::vector<AABB> boxes;
    block->GetBoxes(level->GetBlockData(pos), &boxes);
    for (const AABB &box : boxes)
    {
        const glm::dvec3 &min = box.GetMin();
        const glm::dvec3 &max = box.GetMax();
        if (max.y >= 1.0 && min.x <= 0.0 && min.z <= 0.0 && max.x >= 1.0 && max.z >= 1.0)
        {
            return true;
        }
    }

    return false;
}

void DoorBlock::OnPlace(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    if (IsUpper(data))
    {
        return;
    }

    BlockPos above(pos.x, pos.y + 1, pos.z);
    level->SetBlock(above, this, nullptr, WithPropertyValue(data, m_halfProperty, 1));
    level->RecordBlockChange(above);
}

void DoorBlock::OnClientPlace(Level *level, const BlockPos &pos)
{
    OnPlace(level, pos);
}

void DoorBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    int facing = GetFacing(data);
    int side   = (facing + 2) % 4;
    if (IsOpen(data))
    {
        side = IsHingeRight(data) ? (facing + 1) % 4 : (facing + 3) % 4;
    }

    Direction *direction = s_doorSides[side];
    glm::dvec3 min(0.0, 0.0, 0.0);
    glm::dvec3 max(1.0, 1.0, 1.0);
    if (direction == Direction::north)
    {
        max.z = kThickness;
    }
    else if (direction == Direction::south)
    {
        min.z = 1.0 - kThickness;
    }
    else if (direction == Direction::west)
    {
        max.x = kThickness;
    }
    else
    {
        min.x = 1.0 - kThickness;
    }

    out->push_back(AABB(min, max));
}

Block *DoorBlock::GetPlacementBlock(Direction *face)
{
    return face == Direction::up ? this : nullptr;
}

uint16_t DoorBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (std::fabs(look.x) > std::fabs(look.z))
    {
        facing = look.x > 0.0 ? 1 : 3;
    }
    else
    {
        facing = look.z > 0.0 ? 2 : 0;
    }

    Direction *right = s_doorSides[(facing + 1) % 4];
    Direction *left  = s_doorSides[(facing + 3) % 4];
    BlockPos rightPos(pos.x + right->dx, pos.y, pos.z + right->dz);
    BlockPos leftPos(pos.x + left->dx, pos.y, pos.z + left->dz);

    bool leftDoor   = IsDoorAt(level, leftPos) || IsDoorAt(level, BlockPos(leftPos.x, leftPos.y + 1, leftPos.z));
    bool rightDoor  = IsDoorAt(level, rightPos) || IsDoorAt(level, BlockPos(rightPos.x, rightPos.y + 1, rightPos.z));
    bool hingeRight = (leftDoor && !rightDoor) || CountSolid(level, rightPos) > CountSolid(level, leftPos);

    uint16_t data = WithPropertyValue(0, m_facingProperty, facing);
    return WithPlacementPower(level, pos, WithPropertyValue(data, m_hingeProperty, hingeRight ? 1 : 0));
}

BlockPos DoorBlock::GetOtherPos(const BlockPos &pos, uint16_t data) const
{
    return BlockPos(pos.x, pos.y + (IsUpper(data) ? -1 : 1), pos.z);
}

bool DoorBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;

    BlockPos below(pos.x, pos.y - 1, pos.z);
    BlockPos above(pos.x, pos.y + 1, pos.z);
    if (IsDoorAt(level, pos))
    {
        if (IsUpper(level->GetBlockData(pos)))
        {
            return IsDoorAt(level, below);
        }

        return IsDoorAt(level, above) && HasSturdyTop(level, below);
    }

    if (pos.y < 1 || pos.y + 1 >= Chunk::s_sizeY)
    {
        return false;
    }

    if (!HasSturdyTop(level, below))
    {
        return false;
    }

    Block *aboveBlock = Block::ById(level->GetBlockId(above));
    return !aboveBlock || aboveBlock == Block::ByName("air") || aboveBlock->CanBeReplaced(level->GetBlockData(above));
}

Block::UVRect DoorBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) box;
    *flipU    = false;
    bool side = direction != Direction::up && direction != Direction::down;
    return IsUpper(data) && side ? GetSecondaryAtlasUVRect() : GetAtlasUVRect(direction);
}

bool DoorBlock::IsFaceMirrored(uint16_t data, Direction *direction) const
{
    // Rotation index counts closed plate sides west, north, east, south
    int rotation  = (GetFacing(data) + 3) % 4;
    bool mirrored = false;
    if (IsOpen(data))
    {
        mirrored = (rotation == 0 && direction == Direction::north) || (rotation == 1 && direction == Direction::east) ||
                   (rotation == 2 && direction == Direction::south) || (rotation == 3 && direction == Direction::west);
    }
    else
    {
        mirrored = (rotation == 0 && direction == Direction::east) || (rotation == 1 && direction == Direction::south) ||
                   (rotation == 2 && direction == Direction::west) || (rotation == 3 && direction == Direction::north);
        if (IsHingeRight(data))
        {
            mirrored = !mirrored;
        }
    }

    return mirrored;
}

void DoorBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();

    // Top and bottom map x to u and z to v, sides take the box bounds unmirrored along their horizontal axis
    if (direction == Direction::up)
    {
        *u0 = (float) max.x;
        *u1 = (float) min.x;
        *v0 = (float) max.z;
        *v1 = (float) min.z;
        return;
    }

    if (direction == Direction::down)
    {
        *u0 = (float) max.x;
        *u1 = (float) min.x;
        *v0 = (float) min.z;
        *v1 = (float) max.z;
        return;
    }

    bool alongX = direction == Direction::north || direction == Direction::south;
    *u0         = (float) (alongX ? min.x : min.z);
    *u1         = (float) (alongX ? max.x : max.z);
    if (IsFaceMirrored(data, direction))
    {
        *u0 = 1.0f - *u0;
        *u1 = 1.0f - *u1;
    }
}
