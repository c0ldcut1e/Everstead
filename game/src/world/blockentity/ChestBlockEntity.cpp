#include "ChestBlockEntity.h"

#include <algorithm>
#include <memory>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../Level.h"
#include "../block/ChestBlock.h"
#include "../save/WorldSave.h"

ChestBlockEntity::ChestBlockEntity(Level *level, const BlockPos &pos) : BlockEntity(level, pos), Container(s_rows, 9)
{
    m_openCount   = 0;
    m_openness    = 0.0f;
    m_oldOpenness = 0.0f;
}

const char *ChestBlockEntity::GetId() const
{
    return "chest";
}

void ChestBlockEntity::Tick()
{
    const float speed  = 0.1f;
    const float volume = 0.2f;

    m_oldOpenness = m_openness;
    if (!m_level->IsClientSide())
    {
        return;
    }

    Direction *partner = ((ChestBlock *) Block::ByName("chest"))->GetPartner(m_level->GetBlockData(m_pos));
    bool playsSound    = partner != Direction::north && partner != Direction::west;
    glm::dvec3 center(m_pos.x + (partner == Direction::east ? 1.0 : 0.5), m_pos.y + 0.5, m_pos.z + (partner == Direction::south ? 1.0 : 0.5));
    if (m_openCount > 0 && m_openness == 0.0f && playsSound)
    {
        m_level->PlaySound("random.chestopen", center, volume, m_level->GetRandom().NextFloat() * 0.1f + 0.9f);
    }

    if ((m_openCount == 0 && m_openness > 0.0f) || (m_openCount > 0 && m_openness < 1.0f))
    {
        float oldOpenness = m_openness;
        m_openness        = std::clamp(m_openness + (m_openCount > 0 ? speed : -speed), 0.0f, 1.0f);
        if (m_openness < 0.5f && oldOpenness >= 0.5f && playsSound)
        {
            m_level->PlaySound("random.chestclosed", center, volume, m_level->GetRandom().NextFloat() * 0.1f + 0.9f);
        }
    }
}

void ChestBlockEntity::Save(CompoundTag *tag) const
{
    BlockEntity::Save(tag);

    std::unique_ptr<ListTag> items(new ListTag());
    for (int i = 0; i < GetSize(); i++)
    {
        const ItemInstance *item = GetItem(i);
        if (item->IsEmpty())
        {
            continue;
        }

        std::unique_ptr<CompoundTag> itemTag = WorldSave::CreateItemTag(*item);
        itemTag->PutByte("Slot", (int8_t) i);
        items->Add(std::move(itemTag));
    }

    tag->Put("Items", std::move(items));
}

void ChestBlockEntity::Load(const CompoundTag *tag)
{
    ListTag *items = tag->GetList("Items");
    for (size_t i = 0; items && i < items->Size(); i++)
    {
        const CompoundTag *itemTag = (const CompoundTag *) items->Get(i);
        int slot                   = itemTag->GetByte("Slot");
        if (slot >= 0 && slot < GetSize())
        {
            SetItem(slot, WorldSave::ReadItemTag(itemTag));
        }
    }
}

void ChestBlockEntity::StartOpen()
{
    m_openCount++;
}

void ChestBlockEntity::StopOpen()
{
    m_openCount--;
}

int ChestBlockEntity::GetOpenCount() const
{
    return m_openCount;
}

void ChestBlockEntity::SetOpenCount(int openCount)
{
    m_openCount = openCount;
}

float ChestBlockEntity::GetOpenness(float partialTicks) const
{
    return m_oldOpenness + (m_openness - m_oldOpenness) * partialTicks;
}
