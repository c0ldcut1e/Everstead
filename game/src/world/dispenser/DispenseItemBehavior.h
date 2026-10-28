#pragma once

#include "../block/BlockPos.h"
#include "../item/ItemInstance.h"

#include <glm/vec3.hpp>

class Direction;
class DispenserBlockEntity;
class Item;
class Level;

class DispenseItemBehavior
{
public:
    virtual ~DispenseItemBehavior() = default;

    ItemInstance Dispense(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance item);

    static DispenseItemBehavior *Get(Item *item);
    static DispenseItemBehavior *GetDefault();

protected:
    virtual bool Execute(DispenserBlockEntity *dispenser, Direction *facing, ItemInstance *item);
    virtual void PlaySound(Level *level, const BlockPos &pos, bool success) const;

    static glm::dvec3 GetDispensePosition(const BlockPos &pos, Direction *facing);
    static void Drop(Level *level, const BlockPos &pos, Direction *facing, const ItemInstance &item);
};
