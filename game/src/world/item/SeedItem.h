#pragma once

#include "Item.h"

class SeedItem : public Item
{
public:
    explicit SeedItem(Block *block);

    Block *GetBlock() const override;

private:
    Block *m_block;
};
