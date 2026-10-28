#include "AgeableMob.h"

#include <cmath>
#include <utility>
#include <vector>

#include "../world/Level.h"
#include "../world/item/ItemInstance.h"
#include "../world/item/SpawnEggItem.h"
#include "player/GameType.h"
#include "player/Player.h"

AgeableMob::AgeableMob(Level *level) : Mob(level)
{
    m_age         = 0;
    m_adultWidth  = m_width;
    m_adultHeight = m_height;
}

bool AgeableMob::Interact(Player *player)
{
    ItemInstance *heldItem = player->GetInventory()->GetSelectedItem();
    SpawnEggItem *egg      = heldItem->IsEmpty() ? nullptr : dynamic_cast<SpawnEggItem *>(heldItem->GetItem());
    if (!egg || egg->GetEntityType() != GetType())
    {
        return Mob::Interact(player);
    }

    if (m_level->IsClientSide())
    {
        return true;
    }

    std::unique_ptr<AgeableMob> baby = CreateOffspring(this);
    baby->SetAge(-24000);
    baby->SetPosition(m_position);
    baby->SetRotation(m_yaw, 0.0f);
    m_level->AddEntity(std::move(baby));
    if (!GameType::HasInfiniteItems(player->GetGameType()))
    {
        heldItem->ConsumeOne();
    }

    return true;
}

bool AgeableMob::IsBaby() const
{
    return m_age < 0;
}

void AgeableMob::SetSize(float width, float height)
{
    m_adultWidth  = width;
    m_adultHeight = height;
    float scale   = IsBaby() ? 0.5f : 1.0f;
    Mob::SetSize(width * scale, height * scale);
}

int AgeableMob::GetAge() const
{
    return m_age;
}

void AgeableMob::SetAge(int age)
{
    bool wasBaby = IsBaby();
    m_age        = age;
    if (wasBaby == IsBaby())
    {
        return;
    }

    SetSize(m_adultWidth, m_adultHeight);
    if (IsBaby())
    {
        return;
    }

    // Collision only stops movement into blocks, so a box that grows into a fence or wall has to be pushed out or it walks through
    std::vector<AABB> boxes;
    m_level->GetCollisionBoxes(m_aabb.Inflated(-1e-7), &boxes);
    for (size_t i = 0; i < boxes.size(); i++)
    {
        if (!m_aabb.Intersects(boxes[i]))
        {
            continue;
        }

        double pushX = m_position.x < (boxes[i].GetMin().x + boxes[i].GetMax().x) * 0.5 ? boxes[i].GetMin().x - m_aabb.GetMax().x
                                                                                        : boxes[i].GetMax().x - m_aabb.GetMin().x;
        double pushZ = m_position.z < (boxes[i].GetMin().z + boxes[i].GetMax().z) * 0.5 ? boxes[i].GetMin().z - m_aabb.GetMax().z
                                                                                        : boxes[i].GetMax().z - m_aabb.GetMin().z;
        if (std::abs(pushX) < std::abs(pushZ))
        {
            m_position.x += pushX;
        }
        else
        {
            m_position.z += pushZ;
        }

        UpdateAABB();
    }
}

void AgeableMob::ServerTick()
{
    Mob::ServerTick();
    if (m_age < 0)
    {
        SetAge(m_age + 1);
    }
    else if (m_age > 0)
    {
        m_age--;
    }
}

void AgeableMob::Load(const CompoundTag *tag)
{
    Mob::Load(tag);
    if (tag->Contains("Age"))
    {
        SetAge(tag->GetInt("Age"));
    }
}

void AgeableMob::Save(CompoundTag *tag) const
{
    Mob::Save(tag);
    tag->PutInt("Age", m_age);
}

void AgeableMob::GetMobData(MobData *data) const
{
    Mob::GetMobData(data);
    data->baby = IsBaby();
}

void AgeableMob::ApplyMobData(const MobData &data)
{
    Mob::ApplyMobData(data);
    SetAge(data.baby ? -1 : 0);
}
