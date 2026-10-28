#pragma once

#include <cstdint>

#include "../../entity/EntityType.h"
#include "Item.h"

class BufferedImage;
class Direction;
class Level;
class ServerPlayer;

class SpawnEggItem : public Item
{
public:
    SpawnEggItem(eENTITY_TYPE entityType, uint32_t baseColor, uint32_t spotColor);

    eENTITY_TYPE GetEntityType() const;
    void BuildIcon(BufferedImage *image) const;
    void Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const;
    void Spawn(Level *level, const glm::dvec3 &position) const;

private:
    eENTITY_TYPE m_entityType;
    uint32_t m_baseColor;
    uint32_t m_spotColor;
};
