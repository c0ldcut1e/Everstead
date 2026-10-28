#pragma once

#include "Block.h"

class PoweredBlock : public Block
{
public:
    PoweredBlock(const std::string &texturePath);

    int GetSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;
};
