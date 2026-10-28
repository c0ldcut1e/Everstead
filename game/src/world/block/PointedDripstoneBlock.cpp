#include "PointedDripstoneBlock.h"

#include "../../entity/block/FallingBlock.h"

#include "../../utils/Random.h"
#include "../Level.h"
#include "../chunk/Chunk.h"
#include "../particle/ParticleSpawnParams.h"
#include "GravityBlock.h"
#include "IceBlock.h"

// Hanging only blocks have one texture per thickness and reuse the tip for merged tips
PointedDripstoneBlock::PointedDripstoneBlock(const std::string &texturePrefix, bool hangingOnly)
    : Block(true, texturePrefix + (hangingOnly ? "_tip.png" : "_up_tip.png"))
{
    const char *const directionNames[] = {"up", "down"};
    const char *const thicknessNames[] = {"tip_merge", "tip", "frustum", "middle", "base"};

    m_hangingOnly       = hangingOnly;
    m_landSound         = "pointed_dripstone.land";
    m_landSoundVariants = 5;
    m_directionProperty = AddProperty("vertical_direction", 2);
    m_thicknessProperty = AddProperty("thickness", eThickness_COUNT);
    for (int direction = 0; direction < 2; direction++)
    {
        for (int thickness = 0; thickness < eThickness_COUNT; thickness++)
        {
            std::string path;
            if (hangingOnly)
            {
                path = texturePrefix + "_" + thicknessNames[thickness == eThickness_TIP_MERGE ? eThickness_TIP : thickness] + ".png";
            }
            else
            {
                path = texturePrefix + "_" + directionNames[direction] + "_" + thicknessNames[thickness] + ".png";
            }

            m_variants[direction][thickness] = AddTextureVariant(path);
        }
    }

    SetTransparent(true);
    SetRenderShape(eRenderShape_CROSS);
    SetCrossJitter(eCrossJitter_None);
    SetFlatItem(true);
    SetRandomTicking(hangingOnly);
    SetAABB(AABB(glm::dvec3(0.3125, 0.0, 0.3125), glm::dvec3(0.6875, 1.0, 0.6875)));
}

Direction *PointedDripstoneBlock::GetTipDirection(uint16_t data) const
{
    return GetPropertyValue(data, m_directionProperty) == 0 ? Direction::up : Direction::down;
}

int PointedDripstoneBlock::GetThickness(uint16_t data) const
{
    return GetPropertyValue(data, m_thicknessProperty);
}

uint16_t PointedDripstoneBlock::MakeState(Direction *tipDirection, int thickness) const
{
    uint16_t data = WithPropertyValue(0, m_directionProperty, tipDirection == Direction::up ? 0 : 1);
    return WithPropertyValue(data, m_thicknessProperty, thickness);
}

Direction *PointedDripstoneBlock::GetAttachment(uint16_t data) const
{
    return Direction::Opposite(GetTipDirection(data));
}

bool PointedDripstoneBlock::IsPointingAt(Level *level, const BlockPos &pos, Direction *tipDirection) const
{
    return Block::ById(level->GetBlockId(pos)) == this && GetTipDirection(level->GetBlockData(pos)) == tipDirection;
}

bool PointedDripstoneBlock::IsSupported(Level *level, const BlockPos &pos, Direction *tipDirection) const
{
    Direction *base = Direction::Opposite(tipDirection);
    BlockPos supportPos(pos.x + base->dx, pos.y + base->dy, pos.z + base->dz);
    if (IsPointingAt(level, supportPos, tipDirection))
    {
        return true;
    }

    return Block::ById(level->GetBlockId(supportPos))->IsFaceSturdy(level->GetBlockData(supportPos), tipDirection);
}

// Placement tries both directions, neighbor updates check the real one
bool PointedDripstoneBlock::CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const
{
    (void) attachment;
    if (m_hangingOnly)
    {
        return IsSupported(level, pos, Direction::down);
    }

    return IsSupported(level, pos, Direction::up) || IsSupported(level, pos, Direction::down);
}

int PointedDripstoneBlock::CalculateThickness(Level *level, const BlockPos &pos, Direction *tipDirection) const
{
    Direction *base = Direction::Opposite(tipDirection);
    BlockPos tipPos(pos.x + tipDirection->dx, pos.y + tipDirection->dy, pos.z + tipDirection->dz);
    if (IsPointingAt(level, tipPos, base))
    {
        return eThickness_TIP_MERGE;
    }

    if (!IsPointingAt(level, tipPos, tipDirection))
    {
        return eThickness_TIP;
    }

    int tipThickness = GetThickness(level->GetBlockData(tipPos));
    if (tipThickness == eThickness_TIP || tipThickness == eThickness_TIP_MERGE)
    {
        return eThickness_FRUSTUM;
    }

    BlockPos basePos(pos.x + base->dx, pos.y + base->dy, pos.z + base->dz);
    return IsPointingAt(level, basePos, tipDirection) ? eThickness_MIDDLE : eThickness_BASE;
}

uint16_t PointedDripstoneBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos,
                                                  const glm::dvec3 &look) const
{
    (void) hitPos;

    Direction *tipDirection = Direction::down;
    if (m_hangingOnly)
    {
        return MakeState(tipDirection, CalculateThickness(level, pos, tipDirection));
    }

    if (face == Direction::up || (face != Direction::down && look.y < 0.0))
    {
        tipDirection = Direction::up;
    }

    if (!IsSupported(level, pos, tipDirection))
    {
        tipDirection = Direction::Opposite(tipDirection);
    }

    return MakeState(tipDirection, CalculateThickness(level, pos, tipDirection));
}

void PointedDripstoneBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data           = level->GetBlockData(pos);
    Direction *tipDirection = GetTipDirection(data);
    if (!IsSupported(level, pos, tipDirection))
    {
        if (tipDirection == Direction::down)
        {
            Fall(level, pos, data);
            return;
        }

        DropAsItem(level, pos, data);
        level->SetBlock(pos, Block::ByName("air"));
        level->RecordBlockChange(pos);
        return;
    }

    uint16_t newData = MakeState(tipDirection, CalculateThickness(level, pos, tipDirection));
    if (newData == data)
    {
        return;
    }

    level->SetBlockData(pos, newData);
    level->RecordBlockChange(pos);
    level->NotifyBlockNeighbors(pos);
}

void PointedDripstoneBlock::UpdateShape(Level *level, const BlockPos &pos)
{
    uint16_t data           = level->GetBlockData(pos);
    Direction *tipDirection = GetTipDirection(data);
    if (!IsSupported(level, pos, tipDirection))
    {
        return;
    }

    uint16_t newData = MakeState(tipDirection, CalculateThickness(level, pos, tipDirection));
    if (newData == data)
    {
        return;
    }

    level->SetBlockData(pos, newData);
    BlockPos above(pos.x, pos.y + 1, pos.z);
    BlockPos below(pos.x, pos.y - 1, pos.z);
    if (Block::ById(level->GetBlockId(above)) == this)
    {
        UpdateShape(level, above);
    }

    if (Block::ById(level->GetBlockId(below)) == this)
    {
        UpdateShape(level, below);
    }
}

// We use the emission instead of the light level so heat reaches icicles from the cave floor
bool PointedDripstoneBlock::IsNearHeat(Level *level, const BlockPos &pos) const
{
    const int maxRadius = 15 / 2;
    for (int x = -maxRadius; x <= maxRadius; x++)
    {
        for (int y = -maxRadius; y <= maxRadius; y++)
        {
            for (int z = -maxRadius; z <= maxRadius; z++)
            {
                int emission = Block::ById(level->GetBlockId(BlockPos(pos.x + x, pos.y + y, pos.z + z)))->GetLightEmission();
                int radius   = emission / 2;
                if (emission > IceBlock::s_meltLightLevel && x * x + y * y + z * z <= radius * radius)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void PointedDripstoneBlock::Tick(Level *level, const BlockPos &pos)
{
    if (!IsNearHeat(level, pos))
    {
        return;
    }

    BlockPos root = pos;
    while (root.y < Chunk::s_sizeY - 1 && IsPointingAt(level, BlockPos(root.x, root.y + 1, root.z), Direction::down))
    {
        root.y++;
    }

    Fall(level, root, level->GetBlockData(root));
}

void PointedDripstoneBlock::RandomTick(Level *level, const BlockPos &pos)
{
    if (IsNearHeat(level, pos))
    {
        level->ScheduleBlockForTick(pos, level->GetRandom().NextInt(300, 1800), 0);
    }
}

void PointedDripstoneBlock::OnHeatPlaced(Level *level, const BlockPos &heatPos, int emission)
{
    int radius = emission / 2;
    for (int x = -radius; x <= radius; x++)
    {
        for (int y = -radius; y <= radius; y++)
        {
            for (int z = -radius; z <= radius; z++)
            {
                BlockPos pos(heatPos.x + x, heatPos.y + y, heatPos.z + z);
                if (x * x + y * y + z * z <= radius * radius && Block::ById(level->GetBlockId(pos)) == this)
                {
                    level->ScheduleBlockForTick(pos, level->GetRandom().NextInt(300, 1800), 0);
                }
            }
        }
    }
}

void PointedDripstoneBlock::Fall(Level *level, const BlockPos &pos, uint16_t data)
{
    std::unique_ptr<Entity> fallingBlock(new FallingBlock(level));
    fallingBlock->SetPosition(glm::dvec3(pos.x + 0.5, pos.y, pos.z + 0.5));
    fallingBlock->ReadSpawnData(((int) level->GetBlockId(pos) << 16) | (int) data);
    level->AddEntity(std::move(fallingBlock));
    level->SetBlock(pos, Block::ByName("air"));
    level->RecordBlockChange(pos);
}

void PointedDripstoneBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    uint16_t data = level->GetBlockData(pos);
    if (m_hangingOnly || GetTipDirection(data) != Direction::down || GetThickness(data) != eThickness_TIP || random->NextInt(0, 19) != 0)
    {
        return;
    }

    BlockPos root(pos.x, pos.y + 1, pos.z);
    while (root.y < Chunk::s_sizeY - 1 && IsPointingAt(level, root, Direction::down))
    {
        root.y++;
    }

    Block *above = Block::ById(level->GetBlockId(BlockPos(root.x, root.y + 1, root.z)));
    if (!above || !above->IsLiquid())
    {
        return;
    }

    // The drip plays its sound once it lands
    ParticleSpawnParams params;
    params.type     = above->IsLava() ? eParticleType_DripLavaHang : eParticleType_DripWaterHang;
    params.position = glm::dvec3(pos.x + 0.5, pos.y - 0.06, pos.z + 0.5);
    params.color    = above->IsLava() ? 0xFFFF4915 : 0xFF334CFF;
    params.hasColor = true;
    level->SpawnParticle(params);
}

AABB PointedDripstoneBlock::GetPlacedAABB(const BlockPos &pos, uint16_t data) const
{
    const double radii[eThickness_COUNT] = {3.0, 3.0, 4.0, 5.0, 6.0};

    int thickness = GetThickness(data);
    double radius = radii[thickness] / 16.0;
    double minY   = 0.0;
    double maxY   = 1.0;
    if (thickness == eThickness_TIP)
    {
        if (GetTipDirection(data) == Direction::up)
        {
            maxY = 11.0 / 16.0;
        }
        else
        {
            minY = 5.0 / 16.0;
        }
    }

    return AABB(glm::dvec3(0.5 - radius, minY, 0.5 - radius), glm::dvec3(0.5 + radius, maxY, 0.5 + radius))
            .Translated(glm::dvec3(pos.x, pos.y, pos.z));
}

void PointedDripstoneBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    out->push_back(GetPlacedAABB(BlockPos(0, 0, 0), data));
}

Block::UVRect PointedDripstoneBlock::GetCrossAtlasUVRect(uint16_t data) const
{
    return GetTextureVariantAtlasUVRect(GetCrossTextureVariant(data));
}

int PointedDripstoneBlock::GetCrossTextureVariant(uint16_t data) const
{
    int direction = GetTipDirection(data) == Direction::up ? 0 : 1;
    return m_variants[direction][GetThickness(data)];
}

void PointedDripstoneBlock::SetLandSound(const std::string &landSound, int variants)
{
    m_landSound         = landSound;
    m_landSoundVariants = variants;
}

const std::string &PointedDripstoneBlock::GetLandSound() const
{
    return m_landSound;
}

int PointedDripstoneBlock::GetLandSoundVariants() const
{
    return m_landSoundVariants;
}
