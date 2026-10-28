#include "RedstoneWireBlock.h"

#include <algorithm>

#include "../Level.h"
#include "../particle/ParticleSpawnParams.h"

Direction *const RedstoneWireBlock::s_sides[4] = {Direction::north, Direction::east, Direction::south, Direction::west};

RedstoneWireBlock::RedstoneWireBlock() : Block(false, "assets/texture/block/redstone_dust_cross.png")
{
    m_powerProperty       = AddProperty("power", 16);
    m_sideProperties[0]   = AddProperty("north", 3);
    m_sideProperties[1]   = AddProperty("east", 3);
    m_sideProperties[2]   = AddProperty("south", 3);
    m_sideProperties[3]   = AddProperty("west", 3);
    m_dotProperty         = AddProperty("dot", 2);
    m_lineVariant         = AddTextureVariant("assets/texture/block/redstone_dust_line.png");
    m_verticalLineVariant = AddTextureVariant("assets/texture/block/redstone_dust_line_vertical.png");
    m_shouldSignal        = true;
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetDestroyTime(0.0f);
    SetBreakSound("dig.stone");
    SetFlatItem(true);
}

void RedstoneWireBlock::OnPlace(Level *level, const BlockPos &pos)
{
    UpdateShape(level, pos);
    if (!level->IsClientSide())
    {
        UpdatePower(level, pos);
        NotifyWireNeighbors(level, pos);
    }
}

void RedstoneWireBlock::OnRemoved(Level *level, const BlockPos &pos, uint16_t data)
{
    (void) data;
    if (!level->IsClientSide())
    {
        NotifyWireNeighbors(level, pos);
    }
}

void RedstoneWireBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    UpdateShape(level, pos);
    if (!level->IsClientSide())
    {
        UpdatePower(level, pos);
    }
}

void RedstoneWireBlock::UpdateShape(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    uint16_t next = GetConnections(level, pos, data);
    if (next == data)
    {
        return;
    }

    level->SetBlockData(pos, next);
    level->RecordBlockChange(pos);
    if (!level->IsClientSide() && GetPropertyValue(next, m_powerProperty) > 0)
    {
        level->NotifyBlockNeighbors(pos);
    }
}

bool RedstoneWireBlock::IsUsable(uint16_t data) const
{
    bool connected[4];
    GetConnectedSides(data, connected);
    return !connected[0] && !connected[1] && !connected[2] && !connected[3];
}

bool RedstoneWireBlock::OnUse(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    level->SetBlockData(pos, WithPropertyValue(data, m_dotProperty, 1 - GetPropertyValue(data, m_dotProperty)));
    level->RecordBlockChange(pos);
    if (level->IsClientSide())
    {
        return true;
    }

    level->NotifyBlockNeighbors(pos);
    for (int side = 0; side < 4; side++)
    {
        BlockPos sidePos(pos.x + s_sides[side]->dx, pos.y, pos.z + s_sides[side]->dz);
        if (Block::ById(level->GetBlockId(sidePos))->IsSignalConductor())
        {
            level->NotifyBlockNeighbors(sidePos);
        }
    }

    return true;
}

uint16_t RedstoneWireBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos,
                                              const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;
    (void) look;
    return GetConnections(level, pos, 0);
}

int RedstoneWireBlock::GetSideConnection(Level *level, const BlockPos &pos, Direction *direction) const
{
    BlockPos side(pos.x + direction->dx, pos.y, pos.z + direction->dz);
    Block *sideBlock = Block::ById(level->GetBlockId(side));
    if (sideBlock->IsSignalConductor())
    {
        bool aboveConductor = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)))->IsSignalConductor();
        return !aboveConductor && Block::ById(level->GetBlockId(BlockPos(side.x, side.y + 1, side.z))) == this ? eSideConnection_UP
                                                                                                               : eSideConnection_NONE;
    }

    if (sideBlock == this || sideBlock->CanConnectRedstone(level->GetBlockData(side), direction) ||
        Block::ById(level->GetBlockId(BlockPos(side.x, side.y - 1, side.z))) == this)
    {
        return eSideConnection_SIDE;
    }

    return eSideConnection_NONE;
}

uint16_t RedstoneWireBlock::GetConnections(Level *level, const BlockPos &pos, uint16_t data) const
{
    for (int side = 0; side < 4; side++)
    {
        data = WithPropertyValue(data, m_sideProperties[side], GetSideConnection(level, pos, s_sides[side]));
    }

    return IsUsable(data) ? data : WithPropertyValue(data, m_dotProperty, 0);
}

void RedstoneWireBlock::GetConnectedSides(uint16_t data, bool *connected) const
{
    int count = 0;
    for (int side = 0; side < 4; side++)
    {
        connected[side] = GetPropertyValue(data, m_sideProperties[side]) != eSideConnection_NONE;
        count += connected[side] ? 1 : 0;
    }

    if (count != 1)
    {
        return;
    }

    for (int side = 0; side < 4; side++)
    {
        if (connected[side])
        {
            connected[(side + 2) % 4] = true;
            return;
        }
    }
}

void RedstoneWireBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0, 1.0 / 16.0, 1.0)));
}

void RedstoneWireBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p     = 1.0 / 16.0;
    const double depth = 1.0 / 64.0;
    bool connected[4];
    GetConnectedSides(data, connected);
    bool any = connected[0] || connected[1] || connected[2] || connected[3];
    if (GetPropertyValue(data, m_dotProperty) == 1)
    {
        out->push_back(AABB(glm::dvec3(5.0 * p, 0.0, 5.0 * p), glm::dvec3(11.0 * p, depth, 11.0 * p)));
        return;
    }

    double x0 = !any || connected[3] ? 0.0 : 5.0 * p;
    double x1 = !any || connected[1] ? 1.0 : 11.0 * p;
    double z0 = !any || connected[0] ? 0.0 : 5.0 * p;
    double z1 = !any || connected[2] ? 1.0 : 11.0 * p;
    out->push_back(AABB(glm::dvec3(x0, 0.0, z0), glm::dvec3(x1, depth, z1)));

    if (GetPropertyValue(data, m_sideProperties[0]) == eSideConnection_UP)
    {
        out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0, 1.0, depth)));
    }

    if (GetPropertyValue(data, m_sideProperties[1]) == eSideConnection_UP)
    {
        out->push_back(AABB(glm::dvec3(1.0 - depth, 0.0, 0.0), glm::dvec3(1.0)));
    }

    if (GetPropertyValue(data, m_sideProperties[2]) == eSideConnection_UP)
    {
        out->push_back(AABB(glm::dvec3(0.0, 0.0, 1.0 - depth), glm::dvec3(1.0)));
    }

    if (GetPropertyValue(data, m_sideProperties[3]) == eSideConnection_UP)
    {
        out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(depth, 1.0, 1.0)));
    }
}

Block::UVRect RedstoneWireBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    *flipU = false;
    if (box.GetMax().y > 0.5)
    {
        return GetTextureVariantAtlasUVRect(m_verticalLineVariant);
    }

    bool connected[4];
    GetConnectedSides(data, connected);
    if ((connected[1] || connected[3]) && !connected[0] && !connected[2])
    {
        return GetTextureVariantAtlasUVRect(m_lineVariant);
    }

    if ((connected[0] || connected[2]) && !connected[1] && !connected[3])
    {
        return GetTextureVariantAtlasUVRect(m_verticalLineVariant);
    }

    return GetAtlasUVRect(direction);
}

uint32_t RedstoneWireBlock::GetDataTint(uint16_t data) const
{
    float power = (float) GetPropertyValue(data, m_powerProperty) / 15.0f;
    float r     = power == 0.0f ? 0.3f : power * 0.6f + 0.4f;
    float g     = std::max(0.0f, power * power * 0.7f - 0.5f);
    float b     = std::max(0.0f, power * power * 0.6f - 0.7f);
    return ((uint32_t) (r * 255.0f) << 16) | ((uint32_t) (g * 255.0f) << 8) | (uint32_t) (b * 255.0f);
}

void RedstoneWireBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    uint16_t data = level->GetBlockData(pos);
    if (GetPropertyValue(data, m_powerProperty) == 0)
    {
        return;
    }

    ParticleSpawnParams params;
    params.type = eParticleType_Dust;
    params.position =
            glm::dvec3(pos.x + 0.5 + (random->NextDouble() - 0.5) * 0.2, pos.y + 1.0 / 16.0, pos.z + 0.5 + (random->NextDouble() - 0.5) * 0.2);
    params.color    = 0xFF000000 | GetDataTint(data);
    params.hasColor = true;
    level->SpawnParticle(params);
}

int RedstoneWireBlock::GetSignal(uint16_t data, Direction *direction) const
{
    int power = GetPropertyValue(data, m_powerProperty);
    if (!m_shouldSignal || power == 0 || direction == Direction::up)
    {
        return 0;
    }

    if (direction == Direction::down)
    {
        return power;
    }

    bool connected[4];
    GetConnectedSides(data, connected);
    if (!connected[0] && !connected[1] && !connected[2] && !connected[3])
    {
        return GetPropertyValue(data, m_dotProperty) == 1 ? 0 : power;
    }

    for (int side = 0; side < 4; side++)
    {
        if (s_sides[side] == direction)
        {
            return connected[side] && !connected[(side + 1) % 4] && !connected[(side + 3) % 4] ? power : 0;
        }
    }

    return 0;
}

int RedstoneWireBlock::GetDirectSignal(uint16_t data, Direction *direction) const
{
    return GetSignal(data, direction);
}

bool RedstoneWireBlock::IsSignalSource() const
{
    return true;
}

int RedstoneWireBlock::GetWirePower(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(pos)) == this ? GetPropertyValue(level->GetBlockData(pos), m_powerProperty) : 0;
}

void RedstoneWireBlock::UpdatePower(Level *level, const BlockPos &pos)
{
    m_shouldSignal = false;
    int power      = level->GetBestNeighborSignal(pos);
    m_shouldSignal = true;

    bool aboveConductor = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)))->IsSignalConductor();
    for (int side = 0; side < 4; side++)
    {
        BlockPos sidePos(pos.x + s_sides[side]->dx, pos.y, pos.z + s_sides[side]->dz);
        power = std::max(power, GetWirePower(level, sidePos) - 1);
        if (Block::ById(level->GetBlockId(sidePos))->IsSignalConductor())
        {
            if (!aboveConductor)
            {
                power = std::max(power, GetWirePower(level, BlockPos(sidePos.x, sidePos.y + 1, sidePos.z)) - 1);
            }
        }
        else
        {
            power = std::max(power, GetWirePower(level, BlockPos(sidePos.x, sidePos.y - 1, sidePos.z)) - 1);
        }
    }

    uint16_t data = level->GetBlockData(pos);
    if (power == GetPropertyValue(data, m_powerProperty))
    {
        return;
    }

    level->SetBlockData(pos, WithPropertyValue(data, m_powerProperty, power));
    level->RecordBlockChange(pos);
    NotifyWireNeighbors(level, pos);
}

// Wires reach diagonally and power the block under them, so neighbors of neighbors have to recheck too
void RedstoneWireBlock::NotifyWireNeighbors(Level *level, const BlockPos &pos)
{
    level->NotifyBlockNeighbors(pos);
    for (uint8_t i = 1; i <= 6; i++)
    {
        Direction *direction = Direction::ById(i);
        level->NotifyBlockNeighbors(BlockPos(pos.x + direction->dx, pos.y + direction->dy, pos.z + direction->dz));
    }
}
