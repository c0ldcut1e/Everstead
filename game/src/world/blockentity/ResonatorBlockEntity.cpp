#include "ResonatorBlockEntity.h"

#include "../ServerLevel.h"

ResonatorBlockEntity::ResonatorBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos)
{}

const char *ResonatorBlockEntity::GetId() const
{
    return "resonator";
}

void ResonatorBlockEntity::Tick()
{
    eRESONATOR_MODE completed;
    if (m_level->IsClientSide() || !ResonatorContainer::Tick(&m_level->GetRandom(), &completed))
    {
        return;
    }

    ((ServerLevel *) m_level)->MarkBlockChunkDirty(m_pos);
    if (completed == eResonatorMode_Fracture)
    {
        m_level->PlaySound("resonator.fracture", glm::dvec3(m_pos.x, m_pos.y, m_pos.z) + 0.5, 1.0f, 1.0f);
    }
    else if (completed == eResonatorMode_Stabilize)
    {
        m_level->PlaySound("resonator.stabilize", glm::dvec3(m_pos.x, m_pos.y, m_pos.z) + 0.5, 1.0f, 1.0f);
    }
}

void ResonatorBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);
    ResonatorContainer::Save(tag);
}

void ResonatorBlockEntity::Load(const CompoundTag *tag)
{
    ResonatorContainer::Load(tag);
}
