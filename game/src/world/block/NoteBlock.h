#pragma once

#include "Block.h"

class NoteBlock : public Block
{
public:
    NoteBlock();

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void TriggerEvent(Level *level, const BlockPos &pos, int id) override;
    bool IsUsable(uint16_t data) const override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    void OnAttack(Level *level, const BlockPos &pos) override;

private:
    void PlayNote(Level *level, const BlockPos &pos);

    int m_noteProperty;
    int m_poweredProperty;
};
