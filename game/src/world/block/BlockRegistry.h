#pragma once

#include "../../utils/MappedRegistry.h"
#include "Block.h"

class BlockRegistry : public MappedRegistry<Block *>
{
public:
    static BlockRegistry *GetInstance();
    static void Init();

    uint32_t Register(const ResourceLocation &key, Block *block);
    uint32_t IdOf(Block *block) const;

};
