#include "DispenserBlock.h"

#include "../ServerLevel.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../dispenser/DispenseItemBehavior.h"

#include <glm/common.hpp>

static Direction *const s_dispenserFacings[] = {Direction::down, Direction::up, Direction::north, Direction::south, Direction::west, Direction::east};

DispenserBlock::DispenserBlock(const std::string &frontPath, const std::string &verticalFrontPath)
    : Block(true, "assets/texture/block/furnace_side.png")
{
    m_facingProperty       = AddProperty("facing", 6);
    m_triggeredProperty    = AddProperty("triggered", 2);
    m_frontTexture         = AddTextureVariant(frontPath);
    m_verticalFrontTexture = AddTextureVariant(verticalFrontPath);
    m_topTexture           = AddTextureVariant("assets/texture/block/furnace_top.png");
    SetBreakSound("dig.stone");
}

void DispenserBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide())
    {
        return;
    }

    uint16_t data  = level->GetBlockData(pos);
    bool powered   = level->GetBestNeighborSignal(pos) > 0 || level->GetBestNeighborSignal(BlockPos(pos.x, pos.y + 1, pos.z)) > 0;
    bool triggered = GetPropertyValue(data, m_triggeredProperty) == 1;
    if (powered && !triggered)
    {
        level->ScheduleBlockForTick(pos, 4, 0);
        level->SetBlockData(pos, WithPropertyValue(data, m_triggeredProperty, 1));
        level->RecordBlockChange(pos);
    }
    else if (!powered && triggered)
    {
        level->SetBlockData(pos, WithPropertyValue(data, m_triggeredProperty, 0));
        level->RecordBlockChange(pos);
    }
}

void DispenserBlock::Tick(Level *level, const BlockPos &pos)
{
    if (level->IsClientSide() || Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    DispenserBlockEntity *dispenser = (DispenserBlockEntity *) level->GetBlockEntity(pos);
    int slot                        = dispenser->GetRandomSlot();
    if (slot < 0)
    {
        level->PlaySound("random.click", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 1.0f, 1.2f);
        return;
    }

    DispenseFrom(level, pos, dispenser, slot);
    if (dispenser->GetItem(slot)->IsEmpty())
    {
        dispenser->ClearItem(slot);
    }

    ((ServerLevel *) level)->MarkBlockChunkDirty(pos);
}

void DispenserBlock::DispenseFrom(Level *level, const BlockPos &pos, DispenserBlockEntity *dispenser, int slot)
{
    ItemInstance *item = dispenser->GetItem(slot);
    dispenser->SetItem(slot, DispenseItemBehavior::Get(item->GetItem())->Dispense(dispenser, GetFacing(level->GetBlockData(pos)), *item));
}

void DispenserBlock::OnBreak(Level *level, const BlockPos &pos)
{
    ((ServerLevel *) level)->DropContainerItems(pos, (DispenserBlockEntity *) level->GetBlockEntity(pos));
}

Block::UVRect DispenserBlock::GetAtlasUVRect(Direction *direction, uint16_t data) const
{
    Direction *facing = GetFacing(data);
    if (direction == facing)
    {
        return GetTextureVariantAtlasUVRect(facing->dy != 0 ? m_verticalFrontTexture : m_frontTexture);
    }

    if (facing->dy != 0 || direction->dy != 0)
    {
        return GetTextureVariantAtlasUVRect(m_topTexture);
    }

    return Block::GetAtlasUVRect(direction);
}

uint16_t DispenserBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) level;
    (void) pos;
    (void) face;
    (void) hitPos;
    glm::dvec3 absLook = glm::abs(look);
    int facing         = 0;
    if (absLook.y >= absLook.x && absLook.y >= absLook.z)
    {
        facing = look.y > 0.0 ? 0 : 1;
    }
    else if (absLook.x >= absLook.z)
    {
        facing = look.x > 0.0 ? 4 : 5;
    }
    else
    {
        facing = look.z > 0.0 ? 2 : 3;
    }

    return WithPropertyValue(0, m_facingProperty, facing);
}

uint16_t DispenserBlock::GetItemData() const
{
    return WithPropertyValue(0, m_facingProperty, 3);
}

bool DispenserBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

const char *DispenserBlock::GetBlockEntityId() const
{
    return "dispenser";
}

bool DispenserBlock::HasAnalogOutput() const
{
    return true;
}

int DispenserBlock::GetAnalogOutput(Level *level, const BlockPos &pos) const
{
    return ((DispenserBlockEntity *) level->GetBlockEntity(pos))->GetRedstoneSignal();
}

Direction *DispenserBlock::GetFacing(uint16_t data) const
{
    return s_dispenserFacings[GetPropertyValue(data, m_facingProperty)];
}
