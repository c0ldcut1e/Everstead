#include "ResonantSocketBlock.h"

#include "../ServerLevel.h"
#include "../blockentity/ResonantSocketBlockEntity.h"

ResonantSocketBlock::ResonantSocketBlock() : Block(true, "assets/texture/block/resonant_socket.png")
{}

bool ResonantSocketBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

void ResonantSocketBlock::OnBreak(Level *level, const BlockPos &pos)
{
    ((ServerLevel *) level)->DropContainerItems(pos, (ResonantSocketBlockEntity *) level->GetBlockEntity(pos));
}

const char *ResonantSocketBlock::GetBlockEntityId() const
{
    return "resonant_socket";
}
