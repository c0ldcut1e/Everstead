#include "BowItem.h"

#include <algorithm>
#include <memory>

#include "../../entity/player/ServerPlayer.h"
#include "../../entity/projectile/Arrow.h"
#include "../../utils/Random.h"
#include "../fracture/FractureTraits.h"
#include "ItemInstance.h"

eUSE_ANIM BowItem::GetUseAnimation() const
{
    return eUseAnim_Bow;
}

int BowItem::GetUseDuration() const
{
    return 72000;
}

void BowItem::Release(ServerPlayer *player, ItemInstance *bow) const
{
    float power = player->GetTicksUsingItem() / 20.0f;
    power       = (power * power + power * 2.0f) / 3.0f;
    if (power < 0.1f)
    {
        return;
    }

    power                = std::min(power, 1.0f);
    bool infinite        = GameType::HasInfiniteItems(player->GetGameType());
    ItemInstance *arrows = player->GetInventory()->FindItem(Item::ByName("arrow"));
    if (!infinite && !arrows)
    {
        return;
    }

    Level *level = player->GetLevel();
    std::unique_ptr<Arrow> arrow(new Arrow(level));
    arrow->Shoot(player, power, power == 1.0f, infinite);
    arrow->SetFractures(FractureTraits::GetActive(*bow));
    level->PlaySound("random.bow", player->GetPosition(), 1.0f, 1.0f / (level->GetRandom().NextFloat() * 0.4f + 1.2f) + power * 0.5f);
    level->AddEntity(std::move(arrow));
    if (!infinite)
    {
        bow->HurtAndBreak(1, player);
        arrows->ConsumeOne();
    }
}

void BowItem::SetPullingIcon(int stage, const Icon &icon)
{
    m_pullingIcons[stage] = icon;
}

const Icon &BowItem::GetPullingIcon(int ticksHeld) const
{
    if (ticksHeld >= 18)
    {
        return m_pullingIcons[2];
    }

    if (ticksHeld > 13)
    {
        return m_pullingIcons[1];
    }

    if (ticksHeld > 0)
    {
        return m_pullingIcons[0];
    }

    return m_icon;
}
