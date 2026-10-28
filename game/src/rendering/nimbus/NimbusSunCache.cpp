#ifndef EVERSTEAD_DEDICATED_SERVER

#include "NimbusSunCache.h"

#include <algorithm>

#include "../Framebuffer.h"
#include "../GlStateManager.h"
#include "../RenderDevice.h"
#include "../Shader.h"
#include "../Textures.h"

#include <glm/geometric.hpp>
#include <glm/gtc/type_ptr.hpp>

NimbusSunCache::NimbusSunCache(const NimbusSettings *settings, const NimbusGrid *grid, const DynamicLights *dynamicLights)
{
    m_settings      = settings;
    m_grid          = grid;
    m_dynamicLights = dynamicLights;
    m_sunCount      = 0;
    m_slotsBuilt    = 0;
    m_cleared       = false;
    m_framebuffer   = new Framebuffer(4096, 4096, eTextureFormat_RGBA8);
    m_shader        = new Shader("assets/shader/fog.vert", "assets/shader/nimbus_sun.frag");
    Invalidate();
}

NimbusSunCache::~NimbusSunCache()
{
    delete m_shader;
    delete m_framebuffer;
}

bool NimbusSunCache::Update(const glm::dvec3 &cameraPos)
{
    glm::vec4 suns[DynamicLights::kMaxSuns];
    int sunCount = 0;
    for (size_t i = 0; i < m_dynamicLights->GetLights().size() && sunCount < DynamicLights::kMaxSuns; i++)
    {
        const DynamicLights::Light &light = m_dynamicLights->GetLights()[i];
        if (light.type == DynamicLights::eLightType_Directional && light.occlusion)
        {
            suns[sunCount] = glm::vec4(-glm::normalize(light.direction), light.size);
            sunCount++;
        }
    }

    if (sunCount != m_sunCount || !std::equal(suns, suns + sunCount, m_suns))
    {
        std::copy(suns, suns + sunCount, m_suns);
        m_sunCount = sunCount;
        Invalidate();
    }

    for (size_t i = 0; i < m_grid->GetRebuiltSlots().size(); i++)
    {
        m_dirty[m_grid->GetRebuiltSlots()[i]] = true;
    }

    for (size_t i = 0; i < m_grid->GetShadowChanges().size(); i++)
    {
        MarkShadowed(m_grid->GetShadowChanges()[i]);
    }

    RenderDevice *device = RenderDevice::GetInstance();
    bool drew            = !m_cleared;
    if (!m_cleared)
    {
        // Cells that have not been traced yet read as half lit, so surfaces trace them per pixel instead of trusting the cache
        GlStateManager::SetClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        m_framebuffer->Bind();
        device->ClearAll();
        m_cleared = true;
    }

    m_slotsBuilt = 0;
    if (m_sunCount == 0)
    {
        return drew;
    }

    const SectionPos &gridMin                = m_grid->GetMin();
    const std::vector<SectionPos> &slotOrder = m_grid->GetSlotOrder();
    for (size_t i = 0; i < slotOrder.size() && m_slotsBuilt < m_settings->sunSlotsPerFrame; i++)
    {
        SectionPos pos(gridMin.x + slotOrder[i].x, gridMin.y + slotOrder[i].y, gridMin.z + slotOrder[i].z);
        int slot = NimbusGrid::SlotIndex(pos);
        if (!m_dirty[slot] || !m_grid->HasSection(pos))
        {
            continue;
        }

        if (m_slotsBuilt == 0)
        {
            m_framebuffer->Bind();
            m_shader->Bind();
            Textures::Bind(m_grid->GetTexture(), 0);
            m_grid->Bind(m_shader, cameraPos);
            m_shader->SetInt("u_nimbusOcclusion", 1);
            m_shader->SetInt("u_cacheSamples", m_settings->shadowSamples);
            m_shader->SetVec4Array("u_sunDirections", m_sunCount, glm::value_ptr(m_suns[0]));
            m_shader->SetInt("u_sunCount", m_sunCount);
        }

        int tileX          = (slot & 63) * 64;
        int tileY          = (slot >> 6) * 64;
        glm::dvec3 section = glm::dvec3(pos.x, pos.y, pos.z) * 16.0 - cameraPos;
        device->SetViewport(tileX, tileY, 64, 64);
        m_shader->SetVec3("u_sunSection", (float) section.x, (float) section.y, (float) section.z);
        m_shader->SetVec2("u_sunTile", (float) tileX, (float) tileY);
        device->DrawArrays(ePrimitiveType_Triangles, 0, 3);
        m_dirty[slot] = false;
        m_slotsBuilt++;
    }

    return drew || m_slotsBuilt > 0;
}

// A section only shadows the sections behind it along each sun, so only those get traced again
void NimbusSunCache::MarkShadowed(const SectionPos &pos)
{
    const SectionPos &gridMin                = m_grid->GetMin();
    const std::vector<SectionPos> &slotOrder = m_grid->GetSlotOrder();
    glm::dvec3 blocker                       = glm::dvec3(pos.x, pos.y, pos.z) * 16.0 + 8.0;
    for (size_t i = 0; i < slotOrder.size(); i++)
    {
        SectionPos slotPos(gridMin.x + slotOrder[i].x, gridMin.y + slotOrder[i].y, gridMin.z + slotOrder[i].z);
        glm::dvec3 offset = blocker - (glm::dvec3(slotPos.x, slotPos.y, slotPos.z) * 16.0 + 8.0);
        for (int s = 0; s < m_sunCount; s++)
        {
            glm::dvec3 toLight = glm::dvec3(m_suns[s]);
            double along       = glm::dot(offset, toLight);
            if (along > -28.0 && glm::length(offset - toLight * along) < 28.0 + std::max(along, 0.0) * m_suns[s].w)
            {
                m_dirty[NimbusGrid::SlotIndex(slotPos)] = true;
                break;
            }
        }
    }
}

void NimbusSunCache::Invalidate()
{
    std::fill(m_dirty, m_dirty + NimbusGrid::kSlotCount, true);
}

TextureHandle NimbusSunCache::GetTexture() const
{
    return m_framebuffer->GetColorTexture();
}

int NimbusSunCache::GetSlotsBuilt() const
{
    return m_slotsBuilt;
}

#endif
