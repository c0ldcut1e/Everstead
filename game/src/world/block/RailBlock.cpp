#include "RailBlock.h"

#include "../Level.h"

constexpr int kRailConnections[10][2][3] = {{{0, 0, -1}, {0, 0, 1}},  {{-1, 0, 0}, {1, 0, 0}}, {{-1, 0, 0}, {1, 1, 0}}, {{-1, 1, 0}, {1, 0, 0}},
                                            {{0, 1, -1}, {0, 0, 1}},  {{0, 0, -1}, {0, 1, 1}}, {{1, 0, 0}, {0, 0, 1}},  {{-1, 0, 0}, {0, 0, 1}},
                                            {{-1, 0, 0}, {0, 0, -1}}, {{1, 0, 0}, {0, 0, -1}}};

RailBlock::RailBlock(const std::string &texturePath, const std::string &variantTexturePath, bool canCurve) : Block(false, texturePath)
{
    m_canCurve        = canCurve;
    m_shapeProperty   = AddProperty("shape", canCurve ? 10 : 6);
    m_poweredProperty = canCurve ? -1 : AddProperty("powered", 2);
    m_variantTexture  = AddTextureVariant(variantTexturePath);
    SetTransparent(true);
    SetCollidable(false);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetFlatItem(true);
    SetDestroyTime(0.7f);
    SetBreakSound("dig.stone");
}

void RailBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    int shape = GetShape(data);
    out->push_back(AABB(glm::dvec3(0.0), glm::dvec3(1.0, shape >= 2 && shape <= 5 ? 10.0 / 16.0 : 2.0 / 16.0, 1.0)));
}

void RailBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    (void) data;
    (void) out;
}

void RailBlock::GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const
{
    int shape = GetShape(data);
    glm::dvec3 center(0.5, 1.0 / 16.0, 0.5);
    glm::dvec3 along(0.0, 0.0, -0.5);
    glm::dvec3 across(0.5, 0.0, 0.0);
    if (shape == 1)
    {
        along  = glm::dvec3(0.5, 0.0, 0.0);
        across = glm::dvec3(0.0, 0.0, 0.5);
    }
    else if (shape >= 2 && shape <= 5)
    {
        glm::dvec3 slopes[4] = {glm::dvec3(0.5, 0.5, 0.0), glm::dvec3(-0.5, 0.5, 0.0), glm::dvec3(0.0, 0.5, -0.5), glm::dvec3(0.0, 0.5, 0.5)};
        along                = slopes[shape - 2];
        across               = along.x != 0.0 ? glm::dvec3(0.0, 0.0, 0.5) : glm::dvec3(0.5, 0.0, 0.0);
        center.y += 0.5;
    }

    // The curve texture joins south and east, so the other curves rotate it a quarter turn per shape
    for (int i = 6; i < shape; i++)
    {
        along  = glm::dvec3(-along.z, 0.0, along.x);
        across = glm::dvec3(-across.z, 0.0, across.x);
    }

    out->push_back(RenderPlane{center - along, center + along, across, 0.0f, 0.0f, 1.0f, 1.0f});
}

Block::UVRect RailBlock::GetAtlasUVRect(Direction *direction, uint16_t data) const
{
    if (m_canCurve ? GetShape(data) >= 6 : IsPowered(data))
    {
        return GetTextureVariantAtlasUVRect(m_variantTexture);
    }

    return Block::GetAtlasUVRect(direction);
}

bool RailBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;
    if (!HasTopSupport(level, BlockPos(pos.x, pos.y - 1, pos.z)))
    {
        return false;
    }

    int shape = GetShape(level->GetBlockData(pos));
    if (shape < 2 || shape > 5)
    {
        return true;
    }

    const int *high = kRailConnections[shape][shape == 2 || shape == 5 ? 1 : 0];
    return HasTopSupport(level, BlockPos(pos.x + high[0], pos.y, pos.z + high[2]));
}

void RailBlock::OnPlace(Level *level, const BlockPos &pos)
{
    PlaceTrack(level, pos, true);
}

void RailBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (m_canCurve && level->GetBlockId(pos) == GetId() && CountPotentialConnections(level, pos) == 3)
    {
        PlaceTrack(level, pos, false);
    }
}

int RailBlock::GetShape(uint16_t data) const
{
    return GetPropertyValue(data, m_shapeProperty);
}

bool RailBlock::IsPowered(uint16_t data) const
{
    return m_poweredProperty >= 0 && GetPropertyValue(data, m_poweredProperty) != 0;
}

bool RailBlock::IsRail(Block *block)
{
    return dynamic_cast<RailBlock *>(block) != nullptr;
}

void RailBlock::SetPowered(Level *level, const BlockPos &pos, bool powered)
{
    level->SetBlockData(pos, WithPropertyValue(level->GetBlockData(pos), m_poweredProperty, powered ? 1 : 0));
    level->RecordBlockChange(pos);
}

void RailBlock::LoadTrack(Level *level, const BlockPos &pos, Track *out) const
{
    out->block = (RailBlock *) Block::ById(level->GetBlockId(pos));
    out->pos   = pos;
    SetConnections(out, out->block->GetShape(level->GetBlockData(pos)));
}

bool RailBlock::FindTrack(Level *level, const BlockPos &pos, Track *out) const
{
    const int offsets[3] = {0, 1, -1};
    for (int i = 0; i < 3; i++)
    {
        BlockPos candidate(pos.x, pos.y + offsets[i], pos.z);
        if (IsRailAt(level, candidate))
        {
            LoadTrack(level, candidate, out);
            return true;
        }
    }

    return false;
}

void RailBlock::SetConnections(Track *track, int shape) const
{
    track->connections.clear();
    for (int i = 0; i < 2; i++)
    {
        const int *offset = kRailConnections[shape][i];
        track->connections.push_back(BlockPos(track->pos.x + offset[0], track->pos.y + offset[1], track->pos.z + offset[2]));
    }
}

bool RailBlock::ConnectsTo(const Track &track, const Track &other) const
{
    return HasConnection(track, other.pos.x, other.pos.z);
}

bool RailBlock::HasConnection(const Track &track, int x, int z) const
{
    for (size_t i = 0; i < track.connections.size(); i++)
    {
        if (track.connections[i].x == x && track.connections[i].z == z)
        {
            return true;
        }
    }

    return false;
}

void RailBlock::RemoveSoftConnections(Level *level, Track *track) const
{
    for (size_t i = 0; i < track->connections.size(); i++)
    {
        Track other;
        if (!FindTrack(level, track->connections[i], &other) || !ConnectsTo(other, *track))
        {
            track->connections.erase(track->connections.begin() + i);
            i--;
        }
        else
        {
            track->connections[i] = other.pos;
        }
    }
}

bool RailBlock::CanConnectTo(const Track &track, const Track &other) const
{
    return ConnectsTo(track, other) || track.connections.size() < 2;
}

void RailBlock::ConnectTo(Level *level, Track *track, const Track &other) const
{
    track->connections.push_back(other.pos);
    int x  = track->pos.x;
    int z  = track->pos.z;
    bool n = HasConnection(*track, x, z - 1);
    bool s = HasConnection(*track, x, z + 1);
    bool w = HasConnection(*track, x - 1, z);
    bool e = HasConnection(*track, x + 1, z);

    int shape = n || s ? 0 : -1;
    if (w || e)
    {
        shape = 1;
    }

    if (track->block->m_canCurve)
    {
        if (s && e && !n && !w)
        {
            shape = 6;
        }

        if (s && w && !n && !e)
        {
            shape = 7;
        }

        if (n && w && !s && !e)
        {
            shape = 8;
        }

        if (n && e && !s && !w)
        {
            shape = 9;
        }
    }

    shape         = SlopeShape(level, track->pos, shape < 0 ? 0 : shape);
    uint16_t data = track->block->WithPropertyValue(level->GetBlockData(track->pos), track->block->m_shapeProperty, shape);
    level->SetBlockData(track->pos, data);
    level->RecordBlockChange(track->pos);
}

bool RailBlock::HasNeighborTrack(Level *level, const Track &track, const BlockPos &pos) const
{
    Track neighbor;
    if (!FindTrack(level, pos, &neighbor))
    {
        return false;
    }

    RemoveSoftConnections(level, &neighbor);
    return CanConnectTo(neighbor, track);
}

int RailBlock::CountPotentialConnections(Level *level, const BlockPos &pos) const
{
    BlockPos neighbors[4] = {BlockPos(pos.x, pos.y, pos.z - 1), BlockPos(pos.x, pos.y, pos.z + 1), BlockPos(pos.x - 1, pos.y, pos.z),
                             BlockPos(pos.x + 1, pos.y, pos.z)};
    int count             = 0;
    for (int i = 0; i < 4; i++)
    {
        Track track;
        if (FindTrack(level, neighbors[i], &track))
        {
            count++;
        }
    }

    return count;
}

int RailBlock::SlopeShape(Level *level, const BlockPos &pos, int shape) const
{
    if (shape == 0)
    {
        if (IsRailAt(level, BlockPos(pos.x, pos.y + 1, pos.z - 1)))
        {
            shape = 4;
        }

        if (IsRailAt(level, BlockPos(pos.x, pos.y + 1, pos.z + 1)))
        {
            shape = 5;
        }
    }
    else if (shape == 1)
    {
        if (IsRailAt(level, BlockPos(pos.x + 1, pos.y + 1, pos.z)))
        {
            shape = 2;
        }

        if (IsRailAt(level, BlockPos(pos.x - 1, pos.y + 1, pos.z)))
        {
            shape = 3;
        }
    }

    return shape;
}

void RailBlock::PlaceTrack(Level *level, const BlockPos &pos, bool first) const
{
    if (level->IsClientSide())
    {
        return;
    }

    Track track;
    LoadTrack(level, pos, &track);
    bool n = HasNeighborTrack(level, track, BlockPos(pos.x, pos.y, pos.z - 1));
    bool s = HasNeighborTrack(level, track, BlockPos(pos.x, pos.y, pos.z + 1));
    bool w = HasNeighborTrack(level, track, BlockPos(pos.x - 1, pos.y, pos.z));
    bool e = HasNeighborTrack(level, track, BlockPos(pos.x + 1, pos.y, pos.z));

    int shape = -1;
    if ((n || s) && !w && !e)
    {
        shape = 0;
    }

    if ((w || e) && !n && !s)
    {
        shape = 1;
    }

    if (m_canCurve)
    {
        if (s && e && !n && !w)
        {
            shape = 6;
        }

        if (s && w && !n && !e)
        {
            shape = 7;
        }

        if (n && w && !s && !e)
        {
            shape = 8;
        }

        if (n && e && !s && !w)
        {
            shape = 9;
        }
    }

    if (shape == -1)
    {
        if (n || s)
        {
            shape = 0;
        }

        if (w || e)
        {
            shape = 1;
        }

        // A powered junction prefers the curves the other way round
        if (m_canCurve && level->GetBestNeighborSignal(pos) > 0)
        {
            if (s && e)
            {
                shape = 6;
            }

            if (w && s)
            {
                shape = 7;
            }

            if (e && n)
            {
                shape = 9;
            }

            if (n && w)
            {
                shape = 8;
            }
        }
        else if (m_canCurve)
        {
            if (n && w)
            {
                shape = 8;
            }

            if (e && n)
            {
                shape = 9;
            }

            if (w && s)
            {
                shape = 7;
            }

            if (s && e)
            {
                shape = 6;
            }
        }
    }

    shape = SlopeShape(level, pos, shape < 0 ? 0 : shape);
    SetConnections(&track, shape);
    uint16_t data = WithPropertyValue(level->GetBlockData(pos), m_shapeProperty, shape);
    if (!first && level->GetBlockData(pos) == data)
    {
        return;
    }

    level->SetBlockData(pos, data);
    level->RecordBlockChange(pos);
    for (size_t i = 0; i < track.connections.size(); i++)
    {
        Track neighbor;
        if (!FindTrack(level, track.connections[i], &neighbor))
        {
            continue;
        }

        RemoveSoftConnections(level, &neighbor);
        if (CanConnectTo(neighbor, track))
        {
            ConnectTo(level, &neighbor, track);
        }
    }
}

bool RailBlock::IsRailAt(Level *level, const BlockPos &pos) const
{
    return IsRail(Block::ById(level->GetBlockId(pos)));
}

bool RailBlock::HasTopSupport(Level *level, const BlockPos &pos) const
{
    return Block::ById(level->GetBlockId(pos))->CanSupportRigid(level->GetBlockData(pos), Direction::up);
}
