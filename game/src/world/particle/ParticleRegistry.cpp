#include "ParticleRegistry.h"

std::vector<ParticleDefinition> ParticleRegistry::s_definitions;

void ParticleRegistry::Init()
{
    Clear();

    ParticleDefinition splash;
    splash.type           = eParticleType_Splash;
    splash.location       = "splash";
    splash.texturePath    = "assets/texture/particle/splash.png";
    splash.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    splash.velocitySpread = glm::dvec3(0.02, 0.02, 0.02);
    splash.size           = 0.1f;
    splash.lifetime       = 12;
    splash.gravity        = 1.0f;
    splash.drag           = 0.98f;
    splash.lit            = false;
    splash.collides       = false;
    splash.terrain        = false;
    Register(splash);

    ParticleDefinition terrain;
    terrain.type           = eParticleType_Terrain;
    terrain.location       = "terrain";
    terrain.texturePath    = "";
    terrain.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    terrain.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    terrain.size           = 0.04f;
    terrain.lifetime       = 20;
    terrain.gravity        = 1.0f;
    terrain.drag           = 0.98f;
    terrain.lit            = false;
    terrain.collides       = true;
    terrain.terrain        = true;
    Register(terrain);

    ParticleDefinition itemCrack = terrain;
    itemCrack.type               = eParticleType_ItemCrack;
    itemCrack.location           = "item";
    Register(itemCrack);

    ParticleDefinition depthSuspend;
    depthSuspend.type           = eParticleType_DepthSuspend;
    depthSuspend.location       = "depth_suspend";
    depthSuspend.texturePath    = "assets/texture/particle/depth_suspend.png";
    depthSuspend.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    depthSuspend.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    depthSuspend.size           = 0.2f;
    depthSuspend.lifetime       = 20;
    depthSuspend.gravity        = 0.0f;
    depthSuspend.drag           = 0.99f;
    depthSuspend.lit            = false;
    depthSuspend.collides       = false;
    depthSuspend.terrain        = false;
    Register(depthSuspend);

    ParticleDefinition rain;
    rain.type           = eParticleType_Rain;
    rain.location       = "rain";
    rain.texturePath    = "assets/texture/particle/splash.png";
    rain.velocity       = glm::dvec3(0.0, 0.15, 0.0);
    rain.velocitySpread = glm::dvec3(0.03, 0.05, 0.03);
    rain.size           = 0.08f;
    rain.lifetime       = 8;
    rain.gravity        = 1.5f;
    rain.drag           = 0.98f;
    rain.lit            = false;
    rain.collides       = true;
    rain.terrain        = false;
    Register(rain);

    ParticleDefinition glow;
    glow.type           = eParticleType_Glow;
    glow.location       = "glow";
    glow.texturePath    = "assets/texture/particle/glow.png";
    glow.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    glow.velocitySpread = glm::dvec3(0.01, 0.01, 0.01);
    glow.size           = 0.08f;
    glow.lifetime       = 50;
    glow.gravity        = 0.0f;
    glow.drag           = 0.96f;
    glow.lit            = true;
    glow.collides       = false;
    glow.terrain        = false;
    Register(glow);

    ParticleDefinition happyVillager;
    happyVillager.type           = eParticleType_HappyVillager;
    happyVillager.location       = "happy_villager";
    happyVillager.texturePath    = "assets/texture/particle/happy_villager.png";
    happyVillager.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    happyVillager.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    happyVillager.size           = 0.2f;
    happyVillager.lifetime       = 20;
    happyVillager.gravity        = 0.0f;
    happyVillager.drag           = 0.99f;
    happyVillager.lit            = false;
    happyVillager.collides       = true;
    happyVillager.terrain        = false;
    Register(happyVillager);

    ParticleDefinition crit;
    crit.type           = eParticleType_Crit;
    crit.location       = "crit";
    crit.texturePath    = "assets/texture/particle/crit.png";
    crit.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    crit.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    crit.size           = 0.15f;
    crit.lifetime       = 6;
    crit.gravity        = 0.5f;
    crit.drag           = 0.7f;
    crit.lit            = false;
    crit.collides       = true;
    crit.terrain        = false;
    Register(crit);

    ParticleDefinition explode;
    explode.type           = eParticleType_Explode;
    explode.location       = "explode";
    explode.texturePath    = "assets/texture/particle/explode.png";
    explode.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    explode.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    explode.size           = 0.1f;
    explode.lifetime       = 16;
    explode.gravity        = -0.1f;
    explode.drag           = 0.9f;
    explode.lit            = false;
    explode.collides       = true;
    explode.terrain        = false;
    explode.frames         = 8;
    Register(explode);

    ParticleDefinition smoke;
    smoke.type           = eParticleType_Smoke;
    smoke.location       = "smoke";
    smoke.texturePath    = "assets/texture/particle/explode.png";
    smoke.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    smoke.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    smoke.size           = 0.075f;
    smoke.lifetime       = 8;
    smoke.gravity        = -0.1f;
    smoke.drag           = 0.96f;
    smoke.lit            = false;
    smoke.collides       = true;
    smoke.terrain        = false;
    smoke.frames         = 8;
    Register(smoke);

    ParticleDefinition dust = smoke;
    dust.type               = eParticleType_Dust;
    dust.location           = "dust";
    dust.size               = 0.075f * 0.75f;
    dust.gravity            = 0.0f;
    Register(dust);

    ParticleDefinition flame;
    flame.type           = eParticleType_Flame;
    flame.location       = "flame";
    flame.texturePath    = "assets/texture/particle/flame.png";
    flame.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    flame.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    flame.size           = 0.05f;
    flame.lifetime       = 8;
    flame.gravity        = 0.0f;
    flame.drag           = 0.96f;
    flame.lit            = true;
    flame.collides       = false;
    flame.terrain        = false;
    flame.frames         = 1;
    Register(flame);

    ParticleDefinition heart;
    heart.type           = eParticleType_Heart;
    heart.location       = "heart";
    heart.texturePath    = "assets/texture/particle/heart.png";
    heart.velocity       = glm::dvec3(0.0, 0.1, 0.0);
    heart.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    heart.size           = 0.15f;
    heart.lifetime       = 16;
    heart.gravity        = 0.0f;
    heart.drag           = 0.86f;
    heart.lit            = false;
    heart.collides       = true;
    heart.terrain        = false;
    Register(heart);

    ParticleDefinition note;
    note.type           = eParticleType_Note;
    note.location       = "note";
    note.texturePath    = "assets/texture/particle/note.png";
    note.velocity       = glm::dvec3(0.0, 0.2, 0.0);
    note.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    note.size           = 0.2f;
    note.lifetime       = 6;
    note.gravity        = 0.0f;
    note.drag           = 0.66f;
    note.lit            = false;
    note.collides       = true;
    note.terrain        = false;
    Register(note);

    ParticleDefinition waxOn;
    waxOn.type           = eParticleType_WaxOn;
    waxOn.location       = "wax_on";
    waxOn.texturePath    = "assets/texture/particle/glow.png";
    waxOn.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    waxOn.velocitySpread = glm::dvec3(0.01, 0.01, 0.01);
    waxOn.size           = 0.08f;
    waxOn.lifetime       = 50;
    waxOn.gravity        = 0.0f;
    waxOn.drag           = 0.96f;
    waxOn.lit            = true;
    waxOn.collides       = false;
    waxOn.terrain        = false;
    waxOn.color          = 0xFFE88C14;
    Register(waxOn);

    ParticleDefinition rift;
    rift.type           = eParticleType_Rift;
    rift.location       = "rift";
    rift.texturePath    = "assets/texture/particle/glow.png";
    rift.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    rift.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    rift.size           = 0.15f;
    rift.lifetime       = 60;
    rift.lifetimeSpread = 100;
    rift.gravity        = 0.0f;
    rift.drag           = 0.9f;
    rift.lit            = true;
    rift.collides       = false;
    rift.terrain        = false;
    rift.color          = 0xFFB060FF;
    Register(rift);

    ParticleDefinition riftSpark;
    riftSpark.type           = eParticleType_RiftSpark;
    riftSpark.location       = "rift_spark";
    riftSpark.texturePath    = "assets/texture/particle/glow.png";
    riftSpark.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    riftSpark.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    riftSpark.size           = 0.15f;
    riftSpark.lifetime       = 22;
    riftSpark.gravity        = 0.0f;
    riftSpark.drag           = 0.92f;
    riftSpark.lit            = true;
    riftSpark.collides       = false;
    riftSpark.terrain        = false;
    riftSpark.color          = 0xFFB060FF;
    Register(riftSpark);

    ParticleDefinition largeExplode;
    largeExplode.type           = eParticleType_LargeExplode;
    largeExplode.location       = "large_explode";
    largeExplode.texturePath    = "assets/texture/particle/explosion.png";
    largeExplode.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    largeExplode.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    largeExplode.size           = 2.0f;
    largeExplode.lifetime       = 6;
    largeExplode.gravity        = 0.0f;
    largeExplode.drag           = 1.0f;
    largeExplode.lit            = true;
    largeExplode.collides       = false;
    largeExplode.terrain        = false;
    largeExplode.frames         = 16;
    Register(largeExplode);

    ParticleDefinition hugeExplosion;
    hugeExplosion.type           = eParticleType_HugeExplosion;
    hugeExplosion.location       = "huge_explosion";
    hugeExplosion.texturePath    = "";
    hugeExplosion.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    hugeExplosion.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    hugeExplosion.size           = 1.0f;
    hugeExplosion.lifetime       = 9;
    hugeExplosion.gravity        = 0.0f;
    hugeExplosion.drag           = 1.0f;
    hugeExplosion.lit            = false;
    hugeExplosion.collides       = false;
    hugeExplosion.terrain        = false;
    Register(hugeExplosion);

    RegisterDrip(eParticleType_DripWaterHang, eParticleType_DripWaterFall, eParticleType_DripWaterLand, "drip_water", "pointed_dripstone.drip_water",
                 15, false);
    RegisterDrip(eParticleType_DripLavaHang, eParticleType_DripLavaFall, eParticleType_DripLavaLand, "drip_lava", "pointed_dripstone.drip_lava", 6,
                 true);
}

void ParticleRegistry::RegisterDrip(ePARTICLE_TYPE hangType, ePARTICLE_TYPE fallType, ePARTICLE_TYPE landType, const std::string &name,
                                    const std::string &landSound, int landSoundVariants, bool glowing)
{
    ParticleDefinition hang;
    hang.type           = hangType;
    hang.location       = name + "_hang";
    hang.texturePath    = "assets/texture/particle/drip_hang.png";
    hang.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    hang.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    hang.size           = 0.1f;
    hang.lifetime       = 40;
    hang.gravity        = 0.0f;
    hang.drag           = 0.98f;
    hang.lit            = glowing;
    hang.collides       = false;
    hang.terrain        = false;
    hang.expireType     = fallType;
    Register(hang);

    ParticleDefinition fall;
    fall.type              = fallType;
    fall.location          = name + "_fall";
    fall.texturePath       = "assets/texture/particle/drip_fall.png";
    fall.velocity          = glm::dvec3(0.0, 0.0, 0.0);
    fall.velocitySpread    = glm::dvec3(0.0, 0.0, 0.0);
    fall.size              = 0.1f;
    fall.lifetime          = 200;
    fall.gravity           = 1.5f;
    fall.drag              = 0.98f;
    fall.lit               = glowing;
    fall.collides          = true;
    fall.terrain           = false;
    fall.landType          = landType;
    fall.landSound         = landSound;
    fall.landSoundVariants = landSoundVariants;
    Register(fall);

    ParticleDefinition land;
    land.type           = landType;
    land.location       = name + "_land";
    land.texturePath    = "assets/texture/particle/drip_land.png";
    land.velocity       = glm::dvec3(0.0, 0.0, 0.0);
    land.velocitySpread = glm::dvec3(0.0, 0.0, 0.0);
    land.size           = 0.1f;
    land.lifetime       = 20;
    land.gravity        = 0.0f;
    land.drag           = 0.98f;
    land.lit            = glowing;
    land.collides       = true;
    land.terrain        = false;
    Register(land);
}

void ParticleRegistry::Clear()
{
    s_definitions.clear();
}

const ParticleDefinition *ParticleRegistry::GetDefinition(ePARTICLE_TYPE type)
{
    for (size_t i = 0; i < s_definitions.size(); i++)
    {
        if (s_definitions[i].type == type)
        {
            return &s_definitions[i];
        }
    }

    return nullptr;
}

const std::vector<ParticleDefinition> &ParticleRegistry::GetDefinitions()
{
    return s_definitions;
}

bool ParticleRegistry::IsTerrain(ePARTICLE_TYPE type)
{
    const ParticleDefinition *definition = GetDefinition(type);
    return definition && definition->terrain;
}

void ParticleRegistry::Register(const ParticleDefinition &definition)
{
    s_definitions.push_back(definition);
}
