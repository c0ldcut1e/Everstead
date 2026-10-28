#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ItemAtlas.h"

#include <string>
#include <unordered_map>

#include "../world/item/BowItem.h"
#include "../world/item/ItemRegistry.h"
#include "../world/item/SpawnEggItem.h"
#include "Icon.h"

ItemAtlas *ItemAtlas::GetInstance()
{
    static ItemAtlas instance;
    return &instance;
}

void ItemAtlas::Build()
{
    ItemRegistry *registry = ItemRegistry::GetInstance();
    BowItem *bow           = (BowItem *) Item::ByName("bow");
    std::vector<std::string> iconPaths;
    std::unordered_map<std::string, BufferedImage> icons;
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Item *item = registry->ById(i);
        iconPaths.push_back(item->GetIconPath());
        SpawnEggItem *spawnEgg = dynamic_cast<SpawnEggItem *>(item);
        if (spawnEgg)
        {
            spawnEgg->BuildIcon(&icons[item->GetIconPath()]);
        }
    }

    for (int i = 0; i < BowItem::s_pullingStages; i++)
    {
        iconPaths.push_back("assets/texture/item/bow_pulling_" + std::to_string(i) + ".png");
    }

    m_atlas.Build(iconPaths, icons);
    Texture *texture = m_atlas.GetTexture();
    texture->SetFilterMode(eTextureFilter_Nearest, eTextureFilter_Nearest);
    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Item *item                  = registry->ById(i);
        TextureAtlas::Region region = m_atlas.Get(item->GetIconPath());
        item->SetIcon(Icon(texture, region.u0, region.v0, region.u1, region.v1));
    }

    for (int i = 0; i < BowItem::s_pullingStages; i++)
    {
        TextureAtlas::Region region = m_atlas.Get("assets/texture/item/bow_pulling_" + std::to_string(i) + ".png");
        bow->SetPullingIcon(i, Icon(texture, region.u0, region.v0, region.u1, region.v1));
    }
}

Texture *ItemAtlas::GetTexture() const
{
    return m_atlas.GetTexture();
}

TextureAtlas *ItemAtlas::GetAtlas()
{
    return &m_atlas;
}

#endif
