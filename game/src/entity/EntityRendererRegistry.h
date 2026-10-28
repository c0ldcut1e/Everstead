#pragma once

#include "../utils/TypedRegistry.h"
#include "EntityRenderer.h"
#include "EntityType.h"

class EntityRendererRegistry : public TypedRegistry<eENTITY_TYPE, EntityRenderer *>
{
public:
    static EntityRendererRegistry *GetInstance();
    static void Init();
};
