#include "BoatItem.h"

#include <memory>
#include <utility>

#include "../../entity/Inventory.h"
#include "../../entity/item/Boat.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../utils/Direction.h"
#include "../Level.h"
#include "ItemInstance.h"

BoatItem::BoatItem(int woodType)
{
    m_woodType = woodType;
}

int BoatItem::GetWoodType() const
{
    return m_woodType;
}

void BoatItem::Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const
{
    std::unique_ptr<Boat> boat(new Boat(level));
    boat->SetWoodType(m_woodType);
    boat->SetPosition(glm::dvec3(pos.x + face->dx + 0.5, pos.y + face->dy, pos.z + face->dz + 0.5));
    boat->SetRotation(player->GetYaw(), 0.0f);
    if (level->IntersectsBlock(boat->GetAABB().Inflated(-0.1)))
    {
        return;
    }

    level->AddEntity(std::move(boat));
    if (!GameType::HasInfiniteItems(player->GetGameType()))
    {
        player->GetInventory()->GetSelectedItem()->ConsumeOne();
    }
}
