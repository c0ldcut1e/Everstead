#include "SpawnEggItem.h"

#include <memory>
#include <utility>
#include <vector>

#include "../../entity/EntityFactory.h"
#include "../../entity/Inventory.h"
#include "../../entity/Mob.h"
#include "../../entity/player/GameType.h"
#include "../../entity/player/ServerPlayer.h"
#include "../../rendering/BufferedImage.h"
#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "../Level.h"
#include "ItemInstance.h"

SpawnEggItem::SpawnEggItem(eENTITY_TYPE entityType, uint32_t baseColor, uint32_t spotColor)
{
    m_entityType = entityType;
    m_baseColor  = baseColor;
    m_spotColor  = spotColor;
}

eENTITY_TYPE SpawnEggItem::GetEntityType() const
{
    return m_entityType;
}

void SpawnEggItem::BuildIcon(BufferedImage *image) const
{
    BufferedImage base("assets/texture/item/spawn_egg.png");
    BufferedImage overlay("assets/texture/item/spawn_egg_overlay.png");
    std::vector<uint8_t> pixels               = base.GetPixels();
    const std::vector<uint8_t> &overlayPixels = overlay.GetPixels();
    for (size_t i = 0; i < pixels.size(); i += 4)
    {
        uint32_t color = m_baseColor;
        if (overlayPixels[i + 3] > 0)
        {
            pixels[i]     = overlayPixels[i];
            pixels[i + 1] = overlayPixels[i + 1];
            pixels[i + 2] = overlayPixels[i + 2];
            pixels[i + 3] = overlayPixels[i + 3];
            color         = m_spotColor;
        }

        pixels[i]     = (uint8_t) (pixels[i] * ((color >> 16) & 0xFF) / 255);
        pixels[i + 1] = (uint8_t) (pixels[i + 1] * ((color >> 8) & 0xFF) / 255);
        pixels[i + 2] = (uint8_t) (pixels[i + 2] * (color & 0xFF) / 255);
    }

    image->SetPixels(base.GetPixelWidth(), base.GetPixelHeight(), pixels.data());
}

void SpawnEggItem::Use(Level *level, ServerPlayer *player, const BlockPos &pos, Direction *face) const
{
    Spawn(level, glm::dvec3(pos.x + face->dx + 0.5, pos.y + face->dy, pos.z + face->dz + 0.5));
    if (!GameType::HasInfiniteItems(player->GetGameType()))
    {
        player->GetInventory()->GetSelectedItem()->ConsumeOne();
    }
}

void SpawnEggItem::Spawn(Level *level, const glm::dvec3 &position) const
{
    std::unique_ptr<Entity> entity = EntityFactory::Create(m_entityType, level);
    entity->SetPosition(position);
    entity->SetRotation(level->GetRandom().NextFloat() * 360.0f, 0.0f);
    Mob *mob = (Mob *) entity.get();
    level->AddEntity(std::move(entity));
    mob->FinalizeSpawn();
    mob->PlayAmbientSound();
}
