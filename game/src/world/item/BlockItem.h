#pragma once

#include <string>

#include "Item.h"

class Block;

class BlockItem : public Item
{
public:
    explicit BlockItem(Block *block);

    Block *GetBlock() const override;
    std::string GetDescriptionId() const override;

private:
    Block *m_block;
};
