#include "MelonBlock.h"

MelonBlock::MelonBlock(const std::string &sidePath, const std::string &topPath) : Block(true, sidePath)
{
    SetTexturePath(Direction::up, topPath);
    SetTexturePath(Direction::down, topPath);
}
