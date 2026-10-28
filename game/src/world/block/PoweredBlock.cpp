#include "PoweredBlock.h"

PoweredBlock::PoweredBlock(const std::string &texturePath) : Block(true, texturePath)
{}

int PoweredBlock::GetSignal(uint16_t data, Direction *direction) const
{
    (void) data;
    (void) direction;

    return 15;
}

bool PoweredBlock::IsSignalSource() const
{
    return true;
}
