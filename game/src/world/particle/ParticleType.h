#pragma once

#include <cstddef>
#include <cstdint>

enum ePARTICLE_TYPE : uint16_t
{
    eParticleType_NotSet        = 0,
    eParticleType_Splash        = 1,
    eParticleType_Terrain       = 2,
    eParticleType_DepthSuspend  = 3,
    eParticleType_Rain          = 4,
    eParticleType_DripWaterHang = 5,
    eParticleType_DripWaterFall = 6,
    eParticleType_DripWaterLand = 7,
    eParticleType_DripLavaHang  = 8,
    eParticleType_DripLavaFall  = 9,
    eParticleType_DripLavaLand  = 10,
    eParticleType_Glow          = 11,
    eParticleType_HappyVillager = 12,
    eParticleType_Crit          = 13,
    eParticleType_Explode       = 14,
    eParticleType_Smoke         = 15,
    eParticleType_LargeExplode  = 16,
    eParticleType_HugeExplosion = 17,
    eParticleType_ItemCrack     = 18,
    eParticleType_Flame         = 19,
    eParticleType_Heart         = 20,
    eParticleType_WaxOn         = 21,
    eParticleType_Rift          = 22,
    eParticleType_RiftSpark     = 23,
    eParticleType_Dust          = 24,
    eParticleType_Note          = 25,
};

struct ParticleTypeHash
{
    size_t operator()(ePARTICLE_TYPE type) const
    {
        return (size_t) type;
    }
};
