#pragma once

#include "../../utils/MappedRegistry.h"
#include "Item.h"

class ItemRegistry : public MappedRegistry<Item *>
{
public:
    static ItemRegistry *GetInstance();
    static void Init();
};
