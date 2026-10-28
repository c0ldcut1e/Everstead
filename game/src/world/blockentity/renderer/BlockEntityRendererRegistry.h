#pragma once

#include <string>

#include "../../../utils/TypedRegistry.h"
#include "BlockEntityRenderer.h"

class BlockEntityRendererRegistry : public TypedRegistry<std::string, BlockEntityRenderer *>
{
public:
    static BlockEntityRendererRegistry *GetInstance();
    static void Init();
};
