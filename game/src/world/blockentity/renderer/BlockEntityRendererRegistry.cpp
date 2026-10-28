#ifndef EVERSTEAD_DEDICATED_SERVER

#include "BlockEntityRendererRegistry.h"

#include "ChestRenderer.h"
#include "MobSpawnerRenderer.h"
#include "MovingPistonRenderer.h"
#include "SignRenderer.h"

BlockEntityRendererRegistry *BlockEntityRendererRegistry::GetInstance()
{
    static BlockEntityRendererRegistry instance;
    return &instance;
}

void BlockEntityRendererRegistry::Init()
{
    GetInstance()->RegisterValue("chest", new ChestRenderer());
    GetInstance()->RegisterValue("mob_spawner", new MobSpawnerRenderer());
    GetInstance()->RegisterValue("piston", new MovingPistonRenderer());
    GetInstance()->RegisterValue("sign", new SignRenderer());
}

#endif
