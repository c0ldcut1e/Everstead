#include "Sheep.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

#include "../../world/crafting/Recipes.h"
#include "../../world/inventory/CraftingContainer.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../../world/loot/LootTable.h"
#include "../ai/goal/BreedGoal.h"
#include "../ai/goal/EatTileGoal.h"
#include "../ai/goal/FloatGoal.h"
#include "../ai/goal/FollowParentGoal.h"
#include "../ai/goal/LookAtPlayerGoal.h"
#include "../ai/goal/PanicGoal.h"
#include "../ai/goal/RandomLookAroundGoal.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../ai/goal/TemptGoal.h"
#include "../item/ItemEntity.h"
#include "../player/Player.h"

Sheep::Sheep(Level *level) : Animal(level)
{
    SetSize(0.9f, 1.3f);
    m_attributes.Get(Attribute::s_maxHealth)->SetBaseValue(8.0);
    m_attributes.Get(Attribute::s_movementSpeed)->SetBaseValue(0.25);
    SetHealth(8);
    m_sheared          = false;
    m_eatAnimationTick = 0;
    m_navigation.SetAvoidWater(true);
    m_goalSelector.AddGoal(0, new FloatGoal(this));
    m_goalSelector.AddGoal(1, new PanicGoal(this, 1.52));
    m_goalSelector.AddGoal(2, new BreedGoal(this, 0.92));
    m_goalSelector.AddGoal(3, new TemptGoal(this, 1.0, Item::ByName("wheat")));
    m_goalSelector.AddGoal(4, new FollowParentGoal(this, 1.0));
    m_goalSelector.AddGoal(5, new EatTileGoal(this));
    m_goalSelector.AddGoal(6, new RandomStrollGoal(this, 0.92));
    m_goalSelector.AddGoal(7, new LookAtPlayerGoal(this, 6.0f));
    m_goalSelector.AddGoal(8, new RandomLookAroundGoal(this));

    int roll = GetRandom().NextInt(0, 99);
    if (roll < 5)
    {
        m_color = 15;
    }
    else if (roll < 10)
    {
        m_color = 7;
    }
    else if (roll < 15)
    {
        m_color = 8;
    }
    else if (roll < 18)
    {
        m_color = 12;
    }
    else
    {
        m_color = GetRandom().NextInt(0, 499) == 0 ? 6 : 0;
    }
}

eENTITY_TYPE Sheep::GetType()
{
    return s_type;
}

bool Sheep::Interact(Player *player)
{
    ItemInstance *heldItem = player->GetInventory()->GetSelectedItem();
    if (!heldItem->IsEmpty() && heldItem->GetItem() == Item::ByName("shears") && IsShearable())
    {
        if (!m_level->IsClientSide())
        {
            Shear();
        }

        heldItem->HurtAndBreak(1, player);
    }

    return Animal::Interact(player);
}

bool Sheep::IsShearable() const
{
    return !m_sheared && !IsBaby();
}

void Sheep::Shear()
{
    m_sheared = true;
    int count = GetRandom().NextInt(1, 3);
    for (int i = 0; i < count; i++)
    {
        glm::dvec3 velocity(GetRandom().NextDouble() * 0.2 - 0.1 + (GetRandom().NextFloat() - GetRandom().NextFloat()) * 0.1f,
                            0.2 + GetRandom().NextFloat() * 0.05f,
                            GetRandom().NextDouble() * 0.2 - 0.1 + (GetRandom().NextFloat() - GetRandom().NextFloat()) * 0.1f);
        std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
        itemEntity->Sync(m_position + glm::dvec3(0.0, 1.0, 0.0), velocity, ItemInstance(GetWoolItem()), 0, 10);
        m_level->AddEntity(std::move(itemEntity));
    }
}

std::unique_ptr<AgeableMob> Sheep::CreateOffspring(AgeableMob *partner)
{
    constexpr const char *kDyeNames[] = {"bone_meal",   "orange_dye", "magenta_dye",    "light_blue_dye", "yellow_dye", "lime_dye",
                                         "pink_dye",    "gray_dye",   "light_gray_dye", "cyan_dye",       "purple_dye", "blue_dye",
                                         "cocoa_beans", "green_dye",  "red_dye",        "black_dye"};

    int firstDye  = 15 - m_color;
    int secondDye = 15 - ((Sheep *) partner)->m_color;
    CraftingContainer grid(2, 1);
    grid.SetItem(CraftingContainer::s_gridSlot, ItemInstance(Item::ByName(kDyeNames[firstDye])));
    grid.SetItem(CraftingContainer::s_gridSlot + 1, ItemInstance(Item::ByName(kDyeNames[secondDye])));
    Item *result = Recipes::GetInstance()->GetResult(grid).GetItem();
    int dye      = GetRandom().NextInt(0, 1) == 0 ? firstDye : secondDye;
    for (int i = 0; i < 16; i++)
    {
        if (result == Item::ByName(kDyeNames[i]))
        {
            dye = i;
        }
    }

    std::unique_ptr<Sheep> sheep = std::make_unique<Sheep>(m_level);
    sheep->m_color               = 15 - dye;
    return sheep;
}

void Sheep::Load(const CompoundTag *tag)
{
    Animal::Load(tag);
    if (tag->Contains("Sheared"))
    {
        m_sheared = tag->GetBoolean("Sheared");
    }

    if (tag->Contains("Color"))
    {
        m_color = tag->GetByte("Color") & 0x0F;
    }
}

void Sheep::Save(CompoundTag *tag) const
{
    Animal::Save(tag);
    tag->PutBoolean("Sheared", m_sheared);
    tag->PutByte("Color", (int8_t) m_color);
}

void Sheep::GetMobData(MobData *data) const
{
    Animal::GetMobData(data);
    data->variant = (uint8_t) (m_color | (m_sheared ? 0x10 : 0));
}

void Sheep::ApplyMobData(const MobData &data)
{
    Animal::ApplyMobData(data);
    m_color   = data.variant & 0x0F;
    m_sheared = (data.variant & 0x10) != 0;
}

void Sheep::HandleEntityEvent(uint8_t event)
{
    if (event == kEatGrassEvent)
    {
        m_eatAnimationTick = kEatAnimationTicks;
    }
}

int Sheep::GetColor() const
{
    return m_color;
}

bool Sheep::IsSheared() const
{
    return m_sheared;
}

void Sheep::Ate()
{
    m_sheared = false;
    if (IsBaby())
    {
        SetAge(std::min(m_age + 1200, 0));
    }
}

float Sheep::GetHeadEatPositionScale(float partialTicks) const
{
    if (m_eatAnimationTick <= 0)
    {
        return 0.0f;
    }

    if (m_eatAnimationTick >= 4 && m_eatAnimationTick <= kEatAnimationTicks - 4)
    {
        return 1.0f;
    }

    if (m_eatAnimationTick < 4)
    {
        return ((float) m_eatAnimationTick - partialTicks) / 4.0f;
    }

    return -((float) (m_eatAnimationTick - kEatAnimationTicks) - partialTicks) / 4.0f;
}

float Sheep::GetHeadEatAngleScale(float partialTicks) const
{
    float pi = std::numbers::pi_v<float>;
    if (m_eatAnimationTick > 4 && m_eatAnimationTick <= kEatAnimationTicks - 4)
    {
        float scale = ((float) (m_eatAnimationTick - 4) - partialTicks) / (float) (kEatAnimationTicks - 8);
        return pi * 0.2f + pi * 0.07f * sinf(scale * 28.7f);
    }

    if (m_eatAnimationTick > 0)
    {
        return pi * 0.2f;
    }

    return m_pitch * pi / 180.0f;
}

void Sheep::ClientTick()
{
    m_eatAnimationTick = std::max(0, m_eatAnimationTick - 1);
    Animal::ClientTick();
}

std::string Sheep::GetAmbientSound() const
{
    return "mob.sheep.say" + std::to_string(GetRandom().NextInt(1, 3));
}

std::string Sheep::GetHurtSound() const
{
    return GetAmbientSound();
}

std::string Sheep::GetDeathSound() const
{
    return GetAmbientSound();
}

void Sheep::GetLootProperties(std::vector<LootProperty> *properties) const
{
    properties->push_back({"color", m_color});
    properties->push_back({"sheared", m_sheared ? 1 : 0});
}

Item *Sheep::GetWoolItem() const
{
    constexpr const char *kColorNames[] = {"white",      "orange", "magenta", "light_blue", "yellow", "lime",  "pink", "gray",
                                           "light_gray", "cyan",   "purple",  "blue",       "brown",  "green", "red",  "black"};

    return Item::ByName(std::string(kColorNames[m_color]) + "_wool");
}
