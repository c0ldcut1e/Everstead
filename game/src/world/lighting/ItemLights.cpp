#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ItemLights.h"

#include <algorithm>
#include <cmath>

#include "../../core/Options.h"
#include "../../entity/global/LightningBolt.h"
#include "../../entity/item/ItemEntity.h"
#include "../../entity/player/Player.h"
#include "../../rendering/Shader.h"
#include "../Level.h"
#include "../block/Block.h"
#include "../item/Item.h"
#include "../item/ItemInstance.h"

#include <glm/gtx/norm.hpp>

void ItemLights::Collect(Level *level, const glm::dvec3 &cameraPos, float partialTicks, int setting)
{
    m_lights.clear();
    if (setting == eDynamicLightSetting_Off)
    {
        return;
    }

    const std::vector<std::unique_ptr<Entity>> &entities = level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Entity *entity = entities[i].get();
        if (entity->GetSharedFlag(eEntityFlag_Spectator))
        {
            continue;
        }

        if (entity->GetType() == eEntityType_LightningBolt)
        {
            if (((LightningBolt *) entity)->IsVisible())
            {
                m_lights.push_back({entity->GetRenderPosition(partialTicks) + glm::dvec3(0.0, 1.0, 0.0), glm::dvec3(12.0, 13.0, 15.0)});
            }

            continue;
        }

        const ItemInstance *item = nullptr;
        if (entity->GetType() == eEntityType_Item)
        {
            item = &((ItemEntity *) entity)->GetItem();
        }
        else if (Player *player = dynamic_cast<Player *>(entity))
        {
            item = player->GetInventory()->GetSelectedItem();
        }

        if (!item || item->IsEmpty())
        {
            continue;
        }

        Block *block = item->GetItem()->GetLightBlock();
        if (!block || block->GetLightEmission() == 0)
        {
            continue;
        }

        AABB box            = entity->GetBoundingBox().Translated(entity->GetRenderPosition(partialTicks));
        glm::dvec3 position = (box.GetMin() + box.GetMax()) * 0.5;
        if (setting == eDynamicLightSetting_Fast)
        {
            position = glm::dvec3(floor(position.x) + 0.5, floor(position.y) + 0.5, floor(position.z) + 0.5);
        }

        uint8_t r;
        uint8_t g;
        uint8_t b;
        block->GetLightColor(&r, &g, &b);
        float scale = (float) block->GetLightEmission() / 255.0f;
        m_lights.push_back({position, glm::dvec3(r * scale, g * scale, b * scale)});
    }

    if (m_lights.size() > kMaxLights)
    {
        std::sort(m_lights.begin(), m_lights.end(),
                  [&cameraPos](const Light &a, const Light &b)
                  {
                      return glm::distance2(a.position, cameraPos) < glm::distance2(b.position, cameraPos);
                  });
        m_lights.resize(kMaxLights);
    }
}

void ItemLights::SampleBlockLight(const glm::dvec3 &pos, uint8_t *r, uint8_t *g, uint8_t *b) const
{
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        const glm::dvec3 &light = m_lights[i].position;
        double distance         = sqrt(glm::distance2(pos, light));
        *r                      = std::max(*r, (uint8_t) std::max(0.0, round(m_lights[i].color.x - distance)));
        *g                      = std::max(*g, (uint8_t) std::max(0.0, round(m_lights[i].color.y - distance)));
        *b                      = std::max(*b, (uint8_t) std::max(0.0, round(m_lights[i].color.z - distance)));
    }
}

void ItemLights::Bind(Shader *shader, const glm::dvec3 &cameraPos) const
{
    float positions[kMaxLights * 4];
    float colors[kMaxLights * 4];
    for (size_t i = 0; i < m_lights.size(); i++)
    {
        positions[i * 4]     = (float) (m_lights[i].position.x - cameraPos.x);
        positions[i * 4 + 1] = (float) (m_lights[i].position.y - cameraPos.y);
        positions[i * 4 + 2] = (float) (m_lights[i].position.z - cameraPos.z);
        positions[i * 4 + 3] = 0.0f;
        colors[i * 4]        = (float) m_lights[i].color.x / 15.0f;
        colors[i * 4 + 1]    = (float) m_lights[i].color.y / 15.0f;
        colors[i * 4 + 2]    = (float) m_lights[i].color.z / 15.0f;
        colors[i * 4 + 3]    = 0.0f;
    }

    shader->SetInt("u_itemLightCount", (int) m_lights.size());
    shader->SetVec4Array("u_itemLightPositions", (int) m_lights.size(), positions);
    shader->SetVec4Array("u_itemLightColors", (int) m_lights.size(), colors);
}

const std::vector<ItemLights::Light> &ItemLights::GetLights() const
{
    return m_lights;
}

#endif
