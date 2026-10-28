#include "BlockItem.h"

#include "../block/Block.h"

BlockItem::BlockItem(Block *block)
{
    m_block = block;
}

Block *BlockItem::GetBlock() const
{
    return m_block;
}

std::string BlockItem::GetDescriptionId() const
{
    return m_block->GetResourceLocation().ToLanguageKey("block");
}
