#include "Pig.h"

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

Pig::Pig(Level *level) : Animal(level)
{
    SetSize(0.9f, 0.9f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(10.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.25);
    SetHealth(10);
    m_navigation.SetAvoidWater(true);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(1, new PanicGoal(this, 1.52));
    m_goalSelector.AddGoal(3, new BreedGoal(this, 1.0));
    m_goalSelector.AddGoal(4, new TemptGoal(this, 1.0, Item::ByName("carrot")));
    m_goalSelector.AddGoal(5, new FollowParentGoal(this, 1.12));
    m_goalSelector.AddGoal(6, new RandomStrollGoal(this, 1.0));
    m_goalSelector.AddGoal(7, new LookAtPlayerGoal(this, 6.0f));
    m_goalSelector.AddGoal(8, new RandomLookAroundGoal(this));
}

eENTITY_TYPE Pig::GetType()
{
    return s_type;
}

std::unique_ptr<AgeableMob> Pig::CreateOffspring(AgeableMob *partner)
{
    (void) partner;
    return std::make_unique<Pig>(m_level);
}

bool Pig::IsFood(const ItemInstance &item) const
{
    return item.GetItem() == Item::ByName("carrot");
}

std::string Pig::GetAmbientSound() const
{
    return "mob.pig.say" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Pig::GetHurtSound() const
{
    return GetAmbientSound();
}

std::string Pig::GetDeathSound() const
{
    return "mob.pig.death";
}
