#include "Animal.h"

#include "../../world/block/Block.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../player/GameType.h"
#include "../player/Player.h"

Animal::Animal(Level *level) : AgeableMob(level)
{
    m_inLove = 0;
}

bool Animal::Interact(Player *player)
{
    ItemInstance *heldItem = player->GetInventory()->GetSelectedItem();
    if (heldItem->IsEmpty() || !IsFood(*heldItem) || m_age != 0 || IsInLove())
    {
        return AgeableMob::Interact(player);
    }

    if (m_level->IsClientSide())
    {
        return true;
    }

    if (!GameType::HasInfiniteItems(player->GetGameType()))
    {
        heldItem->ConsumeOne();
    }

    m_inLove = 600;
    return true;
}

bool Animal::Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker)
{
    m_inLove = 0;
    return AgeableMob::Hurt(source, damage, attacker);
}

float Animal::GetWalkTargetValue(const BlockPos &pos) const
{
    if (Block::ById(m_level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z))) == Block::ByName("grass_block"))
    {
        return 10.0f;
    }

    return (float) m_level->GetRawBrightness(pos) - 0.5f;
}

void Animal::Load(const CompoundTag *tag)
{
    AgeableMob::Load(tag);
    if (tag->Contains("InLove"))
    {
        m_inLove = tag->GetInt("InLove");
    }
}

void Animal::Save(CompoundTag *tag) const
{
    AgeableMob::Save(tag);
    tag->PutInt("InLove", m_inLove);
}

void Animal::GetMobData(MobData *data) const
{
    AgeableMob::GetMobData(data);
    data->inLove = IsInLove();
}

void Animal::ApplyMobData(const MobData &data)
{
    AgeableMob::ApplyMobData(data);
    m_inLove = data.inLove ? 1 : 0;
}

bool Animal::IsFood(const ItemInstance &item) const
{
    return item.GetItem() == Item::ByName("wheat");
}

bool Animal::IsInLove() const
{
    return m_inLove > 0;
}

void Animal::ResetLove()
{
    m_inLove = 0;
}

bool Animal::CanMate(Animal *other)
{
    return other != this && other->GetType() == GetType() && IsInLove() && other->IsInLove();
}

void Animal::ServerTick()
{
    AgeableMob::ServerTick();
    if (m_age != 0)
    {
        m_inLove = 0;
    }

    if (m_inLove > 0)
    {
        m_inLove--;
    }
}

void Animal::ClientTick()
{
    AgeableMob::ClientTick();
    if (IsInLove() && m_totalTickCount % 10 == 0)
    {
        ParticleSpawnParams params;
        params.type = eParticleType_Heart;
        params.position =
                glm::dvec3(m_position.x + GetRandom().NextFloat() * m_width * 2.0f - m_width, m_position.y + 0.5 + GetRandom().NextFloat() * m_height,
                           m_position.z + GetRandom().NextFloat() * m_width * 2.0f - m_width);
        m_level->SpawnParticle(params);
    }
}

int Animal::GetAmbientSoundInterval() const
{
    return 120;
}

int Animal::GetExperienceReward()
{
    return 1 + GetRandom().NextInt(0, 2);
}
