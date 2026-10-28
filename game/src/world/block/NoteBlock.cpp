#include "NoteBlock.h"

#include <cmath>
#include <memory>

#include "../../network/packet/s2c/ParticleS2CPacket.h"
#include "../ServerLevel.h"

NoteBlock::NoteBlock() : Block(true, "assets/texture/block/note_block.png")
{
    m_noteProperty    = AddProperty("note", 25);
    m_poweredProperty = AddProperty("powered", 2);
    SetBreakSound("dig.wood");
}

void NoteBlock::OnNeighborChanged(Level *level, const BlockPos &pos)
{
    Block::OnNeighborChanged(level, pos);
    if (level->IsClientSide() || Block::ById(level->GetBlockId(pos)) != this)
    {
        return;
    }

    uint16_t data = level->GetBlockData(pos);
    bool powered  = level->GetBestNeighborSignal(pos) > 0;
    if (powered != (GetPropertyValue(data, m_poweredProperty) != 0))
    {
        if (powered)
        {
            PlayNote(level, pos);
        }

        level->SetBlockData(pos, WithPropertyValue(data, m_poweredProperty, powered ? 1 : 0));
        level->RecordBlockChange(pos);
    }
}

void NoteBlock::TriggerEvent(Level *level, const BlockPos &pos, int id)
{
    (void) id;

    int note     = GetPropertyValue(level->GetBlockData(pos), m_noteProperty);
    Block *below = Block::ById(level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)));
    level->PlaySound(below->GetNoteInstrument(), glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 3.0f, (float) pow(2.0, (note - 12) / 12.0));
    glm::dvec3 particlePos(pos.x + 0.5, pos.y + 1.2, pos.z + 0.5);
    ((ServerLevel *) level)
            ->SendToNearby(
                    particlePos, 64.0,
                    std::shared_ptr<Packet>(new ParticleS2CPacket(eParticleType_Note, particlePos, glm::dvec3(note / 24.0, 0.0, 0.0), 1.0f, 0)));
}

bool NoteBlock::IsUsable(uint16_t data) const
{
    (void) data;

    return true;
}

bool NoteBlock::OnUse(Level *level, const BlockPos &pos)
{
    uint16_t data = level->GetBlockData(pos);
    level->SetBlockData(pos, WithPropertyValue(data, m_noteProperty, (GetPropertyValue(data, m_noteProperty) + 1) % 25));
    level->RecordBlockChange(pos);
    PlayNote(level, pos);
    return true;
}

void NoteBlock::OnAttack(Level *level, const BlockPos &pos)
{
    PlayNote(level, pos);
}

void NoteBlock::PlayNote(Level *level, const BlockPos &pos)
{
    if (level->GetBlockId(BlockPos(pos.x, pos.y + 1, pos.z)) == 0)
    {
        level->AddBlockEvent(pos, this, 0);
    }
}
