#include "FurnaceBlock.h"

#include <cmath>

#include "../../utils/Random.h"
#include "../ServerLevel.h"
#include "../blockentity/FurnaceBlockEntity.h"
#include "../particle/ParticleSpawnParams.h"

static Direction *s_frontDirections[] = {Direction::north, Direction::south, Direction::west, Direction::east};

FurnaceBlock::FurnaceBlock(const std::string &sidePath, const std::string &topPath, const std::string &frontPath, bool container, bool lit)
    : Block(true, sidePath)
{
    m_container      = container;
    m_lit            = lit;
    m_facingProperty = AddProperty("facing", 4);
    SetFaceTexture(Direction::up, topPath);
    SetFaceTexture(Direction::down, topPath);
    SetSecondaryTexture(frontPath);
}

uint16_t FurnaceBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;

    int facing = 0;
    if (fabs(look.x) > fabs(look.z))
    {
        facing = look.x > 0.0 ? 2 : 3;
    }
    else
    {
        facing = look.z > 0.0 ? 0 : 1;
    }

    return WithPropertyValue(0, m_facingProperty, facing);
}

uint16_t FurnaceBlock::GetItemData() const
{
    return WithPropertyValue(0, m_facingProperty, 1);
}

Block::UVRect FurnaceBlock::GetAtlasUVRect(Direction *direction, uint16_t data) const
{
    if (s_frontDirections[GetPropertyValue(data, m_facingProperty)] == direction)
    {
        return GetSecondaryAtlasUVRect();
    }

    return Block::GetAtlasUVRect(direction);
}

bool FurnaceBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return m_container;
}

void FurnaceBlock::OnBreak(Level *level, const BlockPos &pos)
{
    FurnaceBlockEntity *furnace = (FurnaceBlockEntity *) level->GetBlockEntity(pos);
    if (!furnace->IsSwappingBlock())
    {
        ((ServerLevel *) level)->DropContainerItems(pos, furnace);
    }
}

const char *FurnaceBlock::GetBlockEntityId() const
{
    return "furnace";
}

bool FurnaceBlock::HasAnalogOutput() const
{
    return true;
}

int FurnaceBlock::GetAnalogOutput(Level *level, const BlockPos &pos) const
{
    return ((FurnaceBlockEntity *) level->GetBlockEntity(pos))->GetRedstoneSignal();
}

void FurnaceBlock::AnimateTick(Level *level, const BlockPos &pos, Random *random)
{
    if (!m_lit)
    {
        return;
    }

    Direction *front = s_frontDirections[GetPropertyValue(level->GetBlockData(pos), m_facingProperty)];
    double side      = random->NextDouble() * 0.6 - 0.3;
    glm::dvec3 position(pos.x + 0.5 + front->dx * 0.52 + front->dz * side, pos.y + random->NextDouble() * 6.0 / 16.0,
                        pos.z + 0.5 + front->dz * 0.52 + front->dx * side);

    ParticleSpawnParams params;
    params.type     = eParticleType_Smoke;
    params.position = position;
    level->SpawnParticle(params);
    params.type = eParticleType_Flame;
    level->SpawnParticle(params);
}
