#pragma once

#include "Block.h"

class SwitchBlock : public Block
{
public:
    SwitchBlock(const std::string &texturePath);

    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;

protected:
    bool IsPowered(uint16_t data) const;
    void SetPowered(Level *level, const BlockPos &pos, bool powered);

    // Subclasses add this themselves because the property order decides the saved data layout
    int m_poweredProperty;

private:
    void NotifySignalNeighbors(Level *level, const BlockPos &pos, uint16_t data);
};
