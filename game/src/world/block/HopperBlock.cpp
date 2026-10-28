#include "HopperBlock.h"

#include "../Level.h"
#include "../ServerLevel.h"
#include "../blockentity/HopperBlockEntity.h"

static Direction *const s_hopperFacings[] = {Direction::down, Direction::north, Direction::south, Direction::west, Direction::east};

HopperBlock::HopperBlock() : Block(false, "assets/texture/block/hopper_outside.png")
{
    SetBlocksLiquid(true);
    m_facingProperty  = AddProperty("facing", 5);
    m_enabledProperty = AddProperty("enabled", 2);
    m_insideTexture   = AddTextureVariant("assets/texture/block/hopper_inside.png");
    m_topTexture      = AddTextureVariant("assets/texture/block/hopper_top.png");
    m_outsideTexture  = AddTextureVariant("assets/texture/block/hopper_outside.png");
    SetTransparent(true);
    SetCollidable(true);
    SetFlatItem(true);
    SetRenderShape(eRenderShape_BOXES);
    SetInteractionAABB(AABB(glm::dvec3(2.0 / 16.0, 11.0 / 16.0, 2.0 / 16.0), glm::dvec3(14.0 / 16.0, 1.0, 14.0 / 16.0)));
    SetBreakSound("dig.stone");
}

void HopperBlock::OnPlace(Level *level, const BlockPos &pos)
{
    UpdateEnabled(level, pos);
}

void HopperBlock::OnBreak(Level *level, const BlockPos &pos)
{
    ((ServerLevel *) level)->DropContainerItems(pos, (HopperBlockEntity *) level->GetBlockEntity(pos));
}

void HopperBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (Block::ById(level->GetBlockId(pos)) == this)
    {
        UpdateEnabled(level, pos);
    }
}

void HopperBlock::UpdateEnabled(Level *level, const BlockPos &pos)
{
    uint16_t data    = level->GetBlockData(pos);
    uint16_t updated = WithPropertyValue(data, m_enabledProperty, level->GetBestNeighborSignal(pos) == 0 ? 1 : 0);
    if (updated != data)
    {
        level->SetBlockData(pos, updated);
        level->RecordBlockChange(pos);
    }
}

void HopperBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    GetRenderBoxes(data, out);
}

void HopperBlock::GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const
{
    const double p = 1.0 / 16.0;
    out->push_back(AABB(glm::dvec3(0.0, 10.0 * p, 0.0), glm::dvec3(1.0, 11.0 * p, 1.0)));
    out->push_back(AABB(glm::dvec3(0.0, 11.0 * p, 0.0), glm::dvec3(2.0 * p, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(14.0 * p, 11.0 * p, 0.0), glm::dvec3(1.0, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(2.0 * p, 11.0 * p, 0.0), glm::dvec3(14.0 * p, 1.0, 2.0 * p)));
    out->push_back(AABB(glm::dvec3(2.0 * p, 11.0 * p, 14.0 * p), glm::dvec3(14.0 * p, 1.0, 1.0)));
    out->push_back(AABB(glm::dvec3(4.0 * p, 4.0 * p, 4.0 * p), glm::dvec3(12.0 * p, 10.0 * p, 12.0 * p)));

    Direction *facing = GetFacing(data);
    if (facing == Direction::down)
    {
        out->push_back(AABB(glm::dvec3(6.0 * p, 0.0, 6.0 * p), glm::dvec3(10.0 * p, 4.0 * p, 10.0 * p)));
        return;
    }

    glm::dvec3 center(0.5 + facing->dx * 6.0 * p, 6.0 * p, 0.5 + facing->dz * 6.0 * p);
    out->push_back(AABB(center - glm::dvec3(2.0 * p), center + glm::dvec3(2.0 * p)));
}

Block::UVRect HopperBlock::GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const
{
    (void) data;
    *flipU = false;
    if (direction != Direction::up || box.GetMax().y < 0.65)
    {
        return GetTextureVariantAtlasUVRect(m_outsideTexture);
    }

    return GetTextureVariantAtlasUVRect(box.GetMax().y < 0.75 ? m_insideTexture : m_topTexture);
}

uint16_t HopperBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) hitPos;
    (void) look;
    Direction *facing = face == Direction::up ? Direction::down : Direction::Opposite(face);
    for (int i = 0; i < 5; i++)
    {
        if (s_hopperFacings[i] == facing)
        {
            return WithPropertyValue(WithPropertyValue(0, m_facingProperty, i), m_enabledProperty, 1);
        }
    }

    return WithPropertyValue(0, m_enabledProperty, 1);
}

bool HopperBlock::IsUsable(uint16_t data) const
{
    (void) data;
    return true;
}

const char *HopperBlock::GetBlockEntityId() const
{
    return "hopper";
}

bool HopperBlock::HasAnalogOutput() const
{
    return true;
}

int HopperBlock::GetAnalogOutput(Level *level, const BlockPos &pos) const
{
    return ((HopperBlockEntity *) level->GetBlockEntity(pos))->GetRedstoneSignal();
}

Direction *HopperBlock::GetFacing(uint16_t data) const
{
    return s_hopperFacings[GetPropertyValue(data, m_facingProperty)];
}

bool HopperBlock::IsEnabled(uint16_t data) const
{
    return GetPropertyValue(data, m_enabledProperty) == 1;
}
