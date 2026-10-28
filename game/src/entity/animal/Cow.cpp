#include "Cow.h"

#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../ai/goal/BreedGoal.h"
#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/FollowParentGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/PanicGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../ai/goal/TemptGoal.h"
#include "../player/ServerPlayer.h"

Cow::Cow(Level *level) : Animal(level)
{
    SetSize(0.9f, 1.3f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(10.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.25);
    SetHealth(10);
    m_navigation.SetAvoidWater(true);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(1, new PanicGoal(this, 1.52));
    m_goalSelector.AddGoal(2, new BreedGoal(this, 0.8));
    m_goalSelector.AddGoal(3, new TemptGoal(this, 1.0, Item::ByName("wheat")));
    m_goalSelector.AddGoal(4, new FollowParentGoal(this, 1.0));
    m_goalSelector.AddGoal(5, new RandomStrollGoal(this, 0.8));
    m_goalSelector.AddGoal(6, new LookAtPlayerGoal(this, 6.0f));
    m_goalSelector.AddGoal(7, new RandomLookAroundGoal(this));
}

eENTITY_TYPE Cow::GetType()
{
    return s_type;
}

bool Cow::Interact(Player *player)
{
    Inventory *inventory   = player->GetInventory();
    ItemInstance *heldItem = inventory->GetSelectedItem();
    if (heldItem->IsEmpty() || heldItem->GetItem() != Item::ByName("bucket"))
    {
        return Animal::Interact(player);
    }

    if (m_level->IsClientSide())
    {
        return true;
    }

    ItemInstance milk(Item::ByName("milk_bucket"));
    heldItem->ConsumeOne();
    if (heldItem->IsEmpty())
    {
        inventory->SetItem(inventory->GetSelectedSlot(), milk);
        return true;
    }

    inventory->AddItem(milk);
    if (!milk.IsEmpty())
    {
        ((ServerPlayer *) player)->DropItem(milk);
    }

    return true;
}

std::unique_ptr<AgeableMob> Cow::CreateOffspring(AgeableMob *partner)
{
    (void) partner;
    return std::make_unique<Cow>(m_level);
}

std::string Cow::GetAmbientSound() const
{
    return "mob.cow.say" + std::to_string(GetRandom().NextInt(1, 4));
}

std::string Cow::GetHurtSound() const
{
    return "mob.cow.hurt" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Cow::GetDeathSound() const
{
    return GetHurtSound();
}

float Cow::GetSoundVolume() const
{
    return 0.4f;
}
