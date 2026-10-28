#include "ResonantSocketBlockEntity.h"

#include <algorithm>

#include "../ServerLevel.h"
#include "../block/Block.h"
#include "../item/Item.h"

ResonantSocketBlockEntity::ResonantSocketBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos)
{}

const char *ResonantSocketBlockEntity::GetId() const
{
    return "resonant_socket";
}

void ResonantSocketBlockEntity::Tick()
{
    if (m_level->IsClientSide() || GetItem(0)->GetItem() != Item::ByName("resonant_shard"))
    {
        return;
    }

    Block *frames[] = {Block::ByName("broken_fracture_frame"), Block::ByName("fractured_fracture_frame"), Block::ByName("cracked_fracture_frame"),
                       Block::ByName("sealed_fracture_frame")};
    const BlockPos sides[4] = {BlockPos(1, 0, 0), BlockPos(-1, 0, 0), BlockPos(0, 0, 1), BlockPos(0, 0, -1)};
    for (int i = 0; i < 4; i++)
    {
        BlockPos center = m_pos + sides[i];
        BlockPos arms[3];
        int stability[3];
        int count = 0;
        for (int j = 0; j < 4; j++)
        {
            BlockPos arm = center + sides[j];
            if (arm == m_pos)
            {
                continue;
            }

            uint32_t id = m_level->GetBlockId(arm);
            int stage   = 0;
            while (stage < 4 && frames[stage]->GetId() != id)
            {
                stage++;
            }

            if (stage == 4)
            {
                break;
            }

            arms[count]      = arm;
            stability[count] = stage;
            count++;
        }

        if (count < 3)
        {
            continue;
        }

        if (stability[0] == 3 && stability[1] == 3 && stability[2] == 3)
        {
            return;
        }

        ServerLevel *level = (ServerLevel *) m_level;
        for (int j = 0; j < 3; j++)
        {
            level->SetBlock(arms[j], frames[std::min(stability[j] + 1, 3)]);
            level->RecordBlockChange(arms[j]);
        }

        ItemInstance *shard = GetItem(0);
        shard->SetCount(shard->GetCount() - 1);
        if (shard->GetCount() <= 0)
        {
            ClearItem(0);
        }

        level->MarkBlockChunkDirty(m_pos);
        level->PlaySound("fracture.stabilize", glm::dvec3(center.x + 0.5, center.y + 0.5, center.z + 0.5), 1.0f, 1.0f);
        return;
    }
}

void ResonantSocketBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);
    ResonantSocketContainer::Save(tag);
}

void ResonantSocketBlockEntity::Load(const CompoundTag *tag)
{
    ResonantSocketContainer::Load(tag);
}
