#include "SeedItem.h"

SeedItem::SeedItem(Block *block)
{
    m_block = block;
}

Block *SeedItem::GetBlock() const
{
    return m_block;
}
