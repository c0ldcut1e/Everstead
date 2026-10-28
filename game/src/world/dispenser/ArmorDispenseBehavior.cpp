#include "ArmorDispenseBehavior.h"

#include <memory>
#include <vector>

#include "../../core/EversteadServer.h"
#include "../../entity/Inventory.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../network/PlayerList.h"
#include "../../utils/Direction.h"
#include "../ServerLevel.h"
#include "../blockentity/DispenserBlockEntity.h"
#include "../item/Item.h"

bool ArmorDispenseBehavior::Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item)
{
    ServerLevel *level  = (ServerLevel *) dispenser->GetLevel();
    const BlockPos &pos = dispenser->GetPos();
    glm::dvec3 min(pos.x + facing->dx, pos.y + facing->dy, pos.z + facing->dz);
    AABB box(min, min + glm::dvec3(1.0));
    int slot                                                  = item->GetItem()->GetArmorSlot();
    PlayerList *playerList                                    = level->GetServer()->GetPlayerList();
    const std::vector<std::unique_ptr<ServerPlayer>> &players = playerList->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        ServerPlayer *player = players[i].get();
        if (player->GetLevel() == level && player->GetBoundingBox().Intersects(box) && player->GetInventory()->GetArmor(slot)->IsEmpty())
        {
            ItemInstance single(*item);
            single.SetCount(1);
            *player->GetInventory()->GetArmor(slot) = single;
            item->SetCount(item->GetCount() - 1);
            player->SendArmor(slot);
            playerList->SendInventoryTo(player);
            return true;
        }
    }

    return DispenseItemBehavior::Execute(dispenser, facing, item);
}
