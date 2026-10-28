#include "ObserverBlock.h"

#include "../Level.h"

#include <glm/common.hpp>

static Direction *const s_observerFacings[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};

ObserverBlock::ObserverBlock() : Block(true, "assets/texture/block/observer_front.png")
{
    m_facingProperty  = AddProperty("facing", 6);
    m_poweredProperty = AddProperty("powered", 2);
    m_frontTexture    = AddTextureVariant("assets/texture/block/observer_front.png");
    AddTextureVariant("assets/texture/block/observer_front.png#rotated");
    m_backTexture = AddTextureVariant("assets/texture/block/observer_back.png");
    AddTextureVariant("assets/texture/block/observer_back.png#rotated");
    m_backOnTexture = AddTextureVariant("assets/texture/block/observer_back_on.png");
    AddTextureVariant("assets/texture/block/observer_back_on.png#rotated");
    m_sideTexture = AddTextureVariant("assets/texture/block/observer_side.png");
    AddTextureVariant("assets/texture/block/observer_side.png#rotated");
    m_topTexture = AddTextureVariant("assets/texture/block/observer_top.png");
    AddTextureVariant("assets/texture/block/observer_top.png#rotated");
    SetCollidable(true);
    SetRenderShape(eRenderShape_BOXES);
    SetBreakSound("dig.stone");
}

void ObserverBlock::OnPlace(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    if (IsPowered(level->GetBlockData(pos)))
    {
        Tick(level, pos);
    }

    level->ScheduleBlockForTick(pos, 2, 0);
}

void ObserverBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    if (!level->IsClientSide() && IsPowered(data))
    {
        UpdateNeighborsInFront(level, pos, data);
    }
}

void ObserverBlock::OnNeighborStateChanged(Level *level, const BlockPos &pos, Direction *direction)
{
    uint16_t data = level->GetBlockData(pos);
    if (direction == GetFacing(data))
    {
        level->ScheduleBlockForTick(pos, 2, 0);
    }
}

void ObserverBlock::Tick(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    if (IsPowered(data))
    {
        data = WithPropertyValue(data, m_poweredProperty, 0);
    }
    else
    {
        data = WithPropertyValue(data, m_poweredProperty, 1);
        level->ScheduleBlockForTick(pos, 2, 0);
    }

    level->SetBlockData(pos, data);
    level->RecordBlockChange(pos);
    UpdateNeighborsInFront(level, pos, data);
}

void ObserverBlock::UpdateNeighborsInFront(Level *level, const BlockPos &pos, uint16_t data) const
{
    Direction *facing = GetFacing(data);
    BlockPos behind(pos.x - facing->dx, pos.y - facing->dy, pos.z - facing->dz);
    Block::ById(level->GetBlockId(behind))->OnNeighborChanged(level, behind);
    level->NotifyBlockNeighbors(behind);
}

Block::UVRect ObserverBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    *flipU      = false;
    int texture = 0;
    float u0    = 0.0f;
    float v0    = 0.0f;
    float u1    = 0.0f;
    float v1    = 0.0f;
    GetFaceUV(data, box, direction, &texture, &u0, &v0, &u1, &v1);
    return GetTextureVariantAtlasUVRect(texture);
}

void ObserverBlock::AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const
{
    int texture = 0;
    GetFaceUV(data, box, direction, &texture, u0, v0, u1, v1);
}

// This undoes the blockstate rotation, which turns the model around y first and then around x
glm::dvec3 ObserverBlock::ToModel(Direction *facing, const glm::dvec3 &world) const
{
    int turnsY = 0;
    int turnsX = 0;
    if (facing == Direction::south)
    {
        turnsY = 2;
    }
    else if (facing == Direction::east)
    {
        turnsY = 1;
    }
    else if (facing == Direction::west)
    {
        turnsY = 3;
    }
    else if (facing == Direction::up)
    {
        turnsX = 3;
    }
    else if (facing == Direction::down)
    {
        turnsX = 1;
    }

    glm::dvec3 q = world - 0.5;
    for (int i = 0; i < turnsY; i++)
    {
        q = glm::dvec3(q.z, q.y, -q.x);
    }

    for (int i = 0; i < turnsX; i++)
    {
        q = glm::dvec3(q.x, -q.z, q.y);
    }

    return q + 0.5;
}

void ObserverBlock::GetFaceUV(uint16_t data, const AABB &box, Direction *direction, int *texture, float *u0, float *v0, float *u1, float *v1) const
{
    Direction *facing = GetFacing(data);
    glm::dvec3 normal = glm::round(ToModel(facing, glm::dvec3(0.5) + glm::dvec3(direction->dx, direction->dy, direction->dz) * 0.5) - 0.5);
    Direction *face   = direction;
    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *candidate = Direction::ById(i);
        if (candidate->dx == (int) normal.x && candidate->dy == (int) normal.y && candidate->dz == (int) normal.z)
        {
            face = candidate;
        }
    }

    int modelTexture = m_topTexture;
    if (face == Direction::north)
    {
        modelTexture = m_frontTexture;
    }
    else if (face == Direction::south)
    {
        modelTexture = IsPowered(data) ? m_backOnTexture : m_backTexture;
    }
    else if (face == Direction::west || face == Direction::east)
    {
        modelTexture = m_sideTexture;
    }

    glm::dvec3 corners[3];
    GetBoxFaceCorners(box, direction, corners);
    glm::vec2 st[3];
    for (int i = 0; i < 3; i++)
    {
        st[i] = GetModelFaceAxes(face, glm::vec3(ToModel(facing, corners[i])));
        if (face == Direction::up)
        {
            st[i].y = 1.0f - st[i].y;
        }
    }

    ResolveFaceUV(modelTexture, st, texture, u0, v0, u1, v1);
}

uint16_t ObserverBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;
    glm::dvec3 absLook = glm::abs(look);
    Direction *facing  = nullptr;
    if (absLook.y >= absLook.x && absLook.y >= absLook.z)
    {
        facing = look.y > 0.0 ? Direction::up : Direction::down;
    }
    else if (absLook.x >= absLook.z)
    {
        facing = look.x > 0.0 ? Direction::east : Direction::west;
    }
    else
    {
        facing = look.z > 0.0 ? Direction::south : Direction::north;
    }

    return WithFacing(0, facing);
}

uint16_t ObserverBlock::GetItemData() const
{
    return WithFacing(0, Direction::south);
}

int ObserverBlock::GetSignal(uint16_t data, Direction *direction) const
{
    return GetDirectSignal(data, direction);
}

int ObserverBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return IsPowered(data) && direction == Direction::Opposite(GetFacing(data)) ? 15 : 0;
}

bool ObserverBlock::IsSignalSource() const
{
    return true;
}

bool ObserverBlock::CanConnectRedstone(uint16_t data, Direction *direction) const
{
    return direction == GetFacing(data);
}

Direction *ObserverBlock::GetFacing(uint16_t data) const
{
    return s_observerFacings[GetPropertyValue(data, m_facingProperty)];
}

bool ObserverBlock::IsPowered(uint16_t data) const
{
    return GetPropertyValue(data, m_poweredProperty) == 1;
}

uint16_t ObserverBlock::WithFacing(uint16_t data, Direction *facing) const
{
    for (int i = 0; i < 6; i++)
    {
        if (s_observerFacings[i] == facing)
        {
            return WithPropertyValue(data, m_facingProperty, i);
        }
    }

    return data;
}
