#pragma once

#include <cstdint>

enum eENTITY_TYPE : uint16_t
{
    eEntityType_Entity                 = 0,
    eEntityType_LivingEntity           = 1,
    eEntityType_Player                 = 2,
    eEntityType_LocalPlayer            = 3,
    eEntityType_MultiPlayerLocalPlayer = 4,
    eEntityType_ServerPlayer           = 5,
    eEntityType_LightningBolt          = 6,
    eEntityType_RemotePlayer           = 7,
    eEntityType_FallingBlock           = 8,
    eEntityType_Item                   = 9,
    eEntityType_PrimedTnt              = 10,
    eEntityType_ExperienceOrb          = 11,
    eEntityType_Snowball               = 12,
    eEntityType_Mob                    = 13,
    eEntityType_Pig                    = 14,
    eEntityType_Cow                    = 15,
    eEntityType_Sheep                  = 16,
    eEntityType_Chicken                = 17,
    eEntityType_ThrownEgg              = 18,
    eEntityType_Zombie                 = 19,
    eEntityType_Spider                 = 20,
    eEntityType_Creeper                = 21,
    eEntityType_Arrow                  = 22,
    eEntityType_Skeleton               = 23,
    eEntityType_Boat                   = 24,
    eEntityType_Parrot                 = 25,
    eEntityType_Ocelot                 = 26,
    eEntityType_Squid                  = 27,
    eEntityType_LeashKnot              = 28,
    eEntityType_Slime                  = 29,
    eEntityType_Minecart               = 30,
};
