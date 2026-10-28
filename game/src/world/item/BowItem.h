#pragma once

#include "Item.h"

class ItemInstance;
class ServerPlayer;

class BowItem : public Item
{
public:
    eUSE_ANIM GetUseAnimation() const override;
    int GetUseDuration() const override;

    void Release(ServerPlayer *player, ItemInstance *bow) const;
    void SetPullingIcon(int stage, const Icon &icon);
    const Icon &GetPullingIcon(int ticksHeld) const;

    static const int s_pullingStages = 3;

private:
    Icon m_pullingIcons[s_pullingStages];
};
