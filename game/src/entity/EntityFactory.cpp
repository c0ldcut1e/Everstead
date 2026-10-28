#include "EntityFactory.h"

#include "../world/block/Block.h"
#include "../world/block/BlockRegistry.h"
#include "animal/Chicken.h"
#include "animal/Cow.h"
#include "animal/Ocelot.h"
#include "animal/Parrot.h"
#include "animal/Pig.h"
#include "animal/Sheep.h"
#include "animal/Squid.h"
#include "block/FallingBlock.h"
#include "global/LightningBolt.h"
#include "item/Boat.h"
#include "item/ExperienceOrb.h"
#include "item/ItemEntity.h"
#include "item/LeashKnot.h"
#include "item/Minecart.h"
#include "item/PrimedTnt.h"
#include "monster/Creeper.h"
#include "monster/Skeleton.h"
#include "monster/Slime.h"
#include "monster/Spider.h"
#include "monster/Zombie.h"
#include "player/Player.h"
#include "projectile/Arrow.h"
#include "projectile/Snowball.h"
#include "projectile/ThrownEgg.h"

std::vector<EntityFactory::Entry> EntityFactory::s_entries;

static int RandomSpawnData(Random *random)
{
    return (int) random->NextUInt();
}

// A summoned falling block is sand, the spawn data carries the block id in the high 16 bits
static int FallingBlockSpawnData(Random *random)
{
    (void) random;
    return (int) (BlockRegistry::GetInstance()->IdOf(Block::ByName("sand")) << 16);
}

static int ZeroSpawnData(Random *random)
{
    (void) random;
    return 0;
}

static int TntSpawnData(Random *random)
{
    (void) random;
    return 80;
}

void EntityFactory::Init()
{
    Register(
            "falling_block", eEntityType_FallingBlock,
            [](Level *level) -> Entity *
            {
                return new FallingBlock(level);
            },
            FallingBlockSpawnData);
    Register(
            "lightning_bolt", eEntityType_LightningBolt,
            [](Level *level) -> Entity *
            {
                return new LightningBolt(level);
            },
            RandomSpawnData);
    Register(
            "item", eEntityType_Item,
            [](Level *level) -> Entity *
            {
                return new ItemEntity(level);
            },
            ZeroSpawnData);
    Register(
            "tnt", eEntityType_PrimedTnt,
            [](Level *level) -> Entity *
            {
                return new PrimedTnt(level);
            },
            TntSpawnData);
    Register(
            "experience_orb", eEntityType_ExperienceOrb,
            [](Level *level) -> Entity *
            {
                return new ExperienceOrb(level);
            },
            ZeroSpawnData);
    Register(
            "leash_knot", LeashKnot::s_type,
            [](Level *level) -> Entity *
            {
                return new LeashKnot(level);
            },
            ZeroSpawnData);
    Register(
            "snowball", eEntityType_Snowball,
            [](Level *level) -> Entity *
            {
                return new Snowball(level);
            },
            ZeroSpawnData);
    Register(
            "pig", Pig::s_type,
            [](Level *level) -> Entity *
            {
                return new Pig(level);
            },
            ZeroSpawnData);
    Register(
            "cow", Cow::s_type,
            [](Level *level) -> Entity *
            {
                return new Cow(level);
            },
            ZeroSpawnData);
    Register(
            "sheep", Sheep::s_type,
            [](Level *level) -> Entity *
            {
                return new Sheep(level);
            },
            ZeroSpawnData);
    Register(
            "chicken", Chicken::s_type,
            [](Level *level) -> Entity *
            {
                return new Chicken(level);
            },
            ZeroSpawnData);
    Register(
            "egg", ThrownEgg::s_type,
            [](Level *level) -> Entity *
            {
                return new ThrownEgg(level);
            },
            ZeroSpawnData);
    Register(
            "player", Player::s_type,
            [](Level *level) -> Entity *
            {
                return level->CreatePlayer(L"Player");
            },
            ZeroSpawnData);
    Register(
            "zombie", Zombie::s_type,
            [](Level *level) -> Entity *
            {
                return new Zombie(level);
            },
            ZeroSpawnData);
    Register(
            "skeleton", Skeleton::s_type,
            [](Level *level) -> Entity *
            {
                return new Skeleton(level);
            },
            ZeroSpawnData);
    Register(
            "spider", Spider::s_type,
            [](Level *level) -> Entity *
            {
                return new Spider(level);
            },
            ZeroSpawnData);
    Register(
            "creeper", Creeper::s_type,
            [](Level *level) -> Entity *
            {
                return new Creeper(level);
            },
            ZeroSpawnData);
    Register(
            "slime", Slime::s_type,
            [](Level *level) -> Entity *
            {
                return new Slime(level);
            },
            ZeroSpawnData);
    Register(
            "arrow", Arrow::s_type,
            [](Level *level) -> Entity *
            {
                return new Arrow(level);
            },
            ZeroSpawnData);
    Register(
            "parrot", Parrot::s_type,
            [](Level *level) -> Entity *
            {
                return new Parrot(level);
            },
            ZeroSpawnData);
    Register(
            "ocelot", Ocelot::s_type,
            [](Level *level) -> Entity *
            {
                return new Ocelot(level);
            },
            ZeroSpawnData);
    Register(
            "squid", Squid::s_type,
            [](Level *level) -> Entity *
            {
                return new Squid(level);
            },
            ZeroSpawnData);
    Register(
            "boat", Boat::s_type,
            [](Level *level) -> Entity *
            {
                return new Boat(level);
            },
            ZeroSpawnData);
    Register(
            "minecart", Minecart::s_type,
            [](Level *level) -> Entity *
            {
                return new Minecart(level);
            },
            ZeroSpawnData);
}

void EntityFactory::Register(const ResourceLocation &location, eENTITY_TYPE type, Entity *(*create)(Level *level), int (*spawnData)(Random *random))
{
    s_entries.push_back({location, type, create, spawnData});
}

const EntityFactory::Entry *EntityFactory::FindEntry(eENTITY_TYPE type)
{
    for (size_t i = 0; i < s_entries.size(); i++)
    {
        if (s_entries[i].type == type)
        {
            return &s_entries[i];
        }
    }

    return nullptr;
}

std::unique_ptr<Entity> EntityFactory::Create(eENTITY_TYPE type, Level *level)
{
    const Entry *entry = FindEntry(type);
    return entry ? std::unique_ptr<Entity>(entry->create(level)) : nullptr;
}

int EntityFactory::CreateSpawnData(eENTITY_TYPE type, Random *random)
{
    const Entry *entry = FindEntry(type);
    return entry ? entry->spawnData(random) : 0;
}

bool EntityFactory::ByName(const ResourceLocation &location, eENTITY_TYPE *type)
{
    for (size_t i = 0; i < s_entries.size(); i++)
    {
        if (s_entries[i].location == location)
        {
            *type = s_entries[i].type;
            return true;
        }
    }

    return false;
}

const ResourceLocation &EntityFactory::GetResourceLocation(eENTITY_TYPE type)
{
    return FindEntry(type)->location;
}

void EntityFactory::GetSpawnableNames(std::vector<std::string> *names)
{
    for (size_t i = 0; i < s_entries.size(); i++)
    {
        names->push_back(s_entries[i].location.ToString());
    }
}
