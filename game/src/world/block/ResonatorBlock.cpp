#include "ResonatorBlock.h"

#include "../ServerLevel.h"
#include "../blockentity/ResonatorBlockEntity.h"

ResonatorBlock::ResonatorBlock() : Block(true, "assets/texture/block/resonator_side.png")
{
    SetFaceTexture(Direction::up, "assets/texture/block/resonator_top.png");
    SetFaceTexture(Direction::down, "assets/texture/block/resonator_bottom.png");
}

bool ResonatorBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

void ResonatorBlock::OnBreak(Level *level, const BlockPos &pos)
{
    ((ServerLevel *) level)->DropContainerItems(pos, (ResonatorBlockEntity *) level->GetBlockEntity(pos));
}

const char *ResonatorBlock::GetBlockEntityId() const
{
    return "resonator";
}
