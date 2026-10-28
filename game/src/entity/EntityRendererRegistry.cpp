#ifndef EVERSTEAD_DEDICATED_SERVER

#include "EntityRendererRegistry.h"

#include "Entity.h"
#include "LivingEntity.h"
#include "animal/Chicken.h"
#include "animal/ChickenRenderer.h"
#include "animal/Cow.h"
#include "animal/Ocelot.h"
#include "animal/OcelotRenderer.h"
#include "animal/Parrot.h"
#include "animal/ParrotRenderer.h"
#include "animal/Pig.h"
#include "animal/QuadrupedRenderer.h"
#include "animal/Sheep.h"
#include "animal/SheepRenderer.h"
#include "animal/Squid.h"
#include "animal/SquidRenderer.h"
#include "block/FallingBlock.h"
#include "block/FallingBlockRenderer.h"
#include "global/LightningBolt.h"
#include "global/LightningBoltRenderer.h"
#include "item/Boat.h"
#include "item/BoatRenderer.h"
#include "item/ExperienceOrb.h"
#include "item/ExperienceOrbRenderer.h"
#include "item/ItemEntity.h"
#include "item/ItemEntityRenderer.h"
#include "item/LeashKnot.h"
#include "item/LeashKnotRenderer.h"
#include "item/Minecart.h"
#include "item/MinecartRenderer.h"
#include "item/PrimedTnt.h"
#include "item/PrimedTntRenderer.h"
#include "monster/Creeper.h"
#include "monster/CreeperRenderer.h"
#include "monster/Skeleton.h"
#include "monster/SkeletonRenderer.h"
#include "monster/Slime.h"
#include "monster/SlimeRenderer.h"
#include "monster/Spider.h"
#include "monster/SpiderRenderer.h"
#include "monster/Zombie.h"
#include "monster/ZombieRenderer.h"
#include "player/LocalPlayer.h"
#include "player/MultiPlayerLocalPlayer.h"
#include "player/Player.h"
#include "player/PlayerRenderer.h"
#include "player/RemotePlayer.h"
#include "projectile/Arrow.h"
#include "projectile/ArrowRenderer.h"
#include "projectile/Snowball.h"
#include "projectile/ThrownEgg.h"
#include "projectile/ThrownItemRenderer.h"

EntityRendererRegistry *EntityRendererRegistry::GetInstance()
{
    static EntityRendererRegistry instance;
    return &instance;
}

void EntityRendererRegistry::Init()
{
    EntityRendererRegistry *registry = GetInstance();
    registry->RegisterValue(Entity::s_type, new EntityRenderer());
    registry->RegisterValue(LivingEntity::s_type, new EntityRenderer());
    registry->RegisterValue(Player::s_type, new EntityRenderer());
    registry->RegisterValue(LocalPlayer::s_type, new EntityRenderer());
    registry->RegisterValue(MultiPlayerLocalPlayer::s_type, new EntityRenderer());
    registry->RegisterValue(RemotePlayer::s_type, new PlayerRenderer());
    registry->RegisterValue(LightningBolt::s_type, new LightningBoltRenderer());
    registry->RegisterValue(FallingBlock::s_type, new FallingBlockRenderer());
    registry->RegisterValue(ItemEntity::s_type, new ItemEntityRenderer());
    registry->RegisterValue(PrimedTnt::s_type, new PrimedTntRenderer());
    registry->RegisterValue(ExperienceOrb::s_type, new ExperienceOrbRenderer());
    registry->RegisterValue(Snowball::s_type, new ThrownItemRenderer());
    registry->RegisterValue(ThrownEgg::s_type, new ThrownItemRenderer());
    registry->RegisterValue(Pig::s_type, new QuadrupedRenderer("pig", 0.7f, 4.0f, 4.0f));
    registry->RegisterValue(Cow::s_type, new QuadrupedRenderer("cow", 0.7f, 8.0f, 6.0f));
    registry->RegisterValue(Sheep::s_type, new SheepRenderer());
    registry->RegisterValue(Chicken::s_type, new ChickenRenderer());
    registry->RegisterValue(Parrot::s_type, new ParrotRenderer());
    registry->RegisterValue(Ocelot::s_type, new OcelotRenderer());
    registry->RegisterValue(Squid::s_type, new SquidRenderer());
    registry->RegisterValue(Zombie::s_type, new ZombieRenderer());
    registry->RegisterValue(Skeleton::s_type, new SkeletonRenderer());
    registry->RegisterValue(Spider::s_type, new SpiderRenderer());
    registry->RegisterValue(Creeper::s_type, new CreeperRenderer());
    registry->RegisterValue(Slime::s_type, new SlimeRenderer());
    registry->RegisterValue(Arrow::s_type, new ArrowRenderer());
    registry->RegisterValue(Boat::s_type, new BoatRenderer());
    registry->RegisterValue(Minecart::s_type, new MinecartRenderer());
    registry->RegisterValue(LeashKnot::s_type, new LeashKnotRenderer());
}

#endif
