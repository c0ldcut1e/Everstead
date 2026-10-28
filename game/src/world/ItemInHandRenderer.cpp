#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ItemInHandRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <numbers>
#include <tuple>
#include <utility>
#include <vector>

#include "../core/Everstead.h"
#include "../entity/player/LocalPlayer.h"
#include "../rendering/BufferBuilder.h"
#include "../rendering/GameRenderer.h"
#include "../rendering/GlStateManager.h"
#include "../rendering/Icon.h"
#include "../rendering/ItemAtlas.h"
#include "../rendering/Tesselator.h"
#include "../rendering/Texture.h"
#include "../rendering/Textures.h"
#include "../utils/AABB.h"
#include "Level.h"
#include "block/Block.h"
#include "block/BlockRenderer.h"
#include "fracture/FractureTraits.h"
#include "item/Item.h"
#include "item/ItemInstance.h"
#include "lighting/Lighting.h"
#include "models/Model.h"
#include "models/ModelDefinition.h"
#include "models/ModelPartCubeDefinition.h"
#include "models/ModelPartDefinition.h"
#include "models/ModelPartSkin.h"
#include "models/ModelRegistry.h"

const float ItemInHandRenderer::s_spriteDepth = 1.0f / 16.0f;

const double ItemInHandRenderer::s_thinBlockHeight = 0.25;

std::map<std::tuple<const Texture *, float, float>, std::vector<BufferBuilder::Vtx>> ItemInHandRenderer::s_spriteMeshes;
const float ItemInHandRenderer::s_equipSpeed = 0.4f;

ItemInHandRenderer::ItemInHandRenderer()
{
    m_armModel       = nullptr;
    m_arm            = nullptr;
    m_armTexturePath = "";
    m_armSkinModel   = ePlayerSkinModel_Classic;

    m_equipHeight      = 0.0f;
    m_equipHeightOld   = 0.0f;
    m_lastSlot         = -1;
    m_selectedItem     = nullptr;
    m_selectedAuxValue = 0;
}

ItemInHandRenderer::~ItemInHandRenderer()
{
    delete m_armModel;
    m_armModel = nullptr;
}

void ItemInHandRenderer::Tick(LocalPlayer *player)
{
    m_equipHeightOld = m_equipHeight;

    Inventory *inventory = player->GetInventory();
    ItemInstance *next   = inventory->GetSelectedItem();
    Item *nextItem       = (next && !next->IsEmpty()) ? next->GetItem() : nullptr;
    int nextAuxValue     = next ? next->GetAuxValue() : 0;

    // We keep the item raised across same item slot switches and wear on the held item
    bool matches = m_selectedItem == nextItem && (m_selectedAuxValue == nextAuxValue || m_lastSlot == inventory->GetSelectedSlot());
    if (matches)
    {
        m_selectedAuxValue = nextAuxValue;
        m_lastSlot         = inventory->GetSelectedSlot();
    }

    float target = matches ? 1.0f : 0.0f;
    float delta  = target - m_equipHeight;
    if (delta < -s_equipSpeed)
    {
        delta = -s_equipSpeed;
    }

    if (delta > s_equipSpeed)
    {
        delta = s_equipSpeed;
    }

    m_equipHeight += delta;

    if (m_equipHeight < 0.1f)
    {
        m_selectedItem     = nextItem;
        m_selectedAuxValue = nextAuxValue;
        m_lastSlot         = inventory->GetSelectedSlot();
    }
}

void ItemInHandRenderer::ResetEquip()
{
    m_equipHeight = 0.0f;
}

void ItemInHandRenderer::Render(const glm::dmat4 &projection, LocalPlayer *player, float partialTicks, uint32_t lightColor)
{
    EnsureArmModel(player);

    float equipHeight = m_equipHeightOld + (m_equipHeight - m_equipHeightOld) * partialTicks;

    GlStateManager::MatrixMode(1);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(projection);

    GlStateManager::MatrixMode(0);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::MultMatrix(Everstead::GetInstance()->GetGameRenderer()->GetViewBobbing(player, partialTicks));

    GlStateManager::DisableDepthTest();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::EnableCull();

    float yaw = player->GetYaw();
    if (Entity *vehicle = player->GetVehicle())
    {
        yaw += vehicle->GetRenderYaw(partialTicks) - vehicle->GetYaw();
    }

    float pitch          = player->GetPitch();
    float tilt           = player->GetTiltOld() + (player->GetTilt() - player->GetTiltOld()) * partialTicks;
    float bob            = player->GetBobOld() + (player->GetBob() - player->GetBobOld()) * partialTicks;
    float yawDelta       = yaw - tilt;
    float pitchDelta     = bob - pitch;
    Everstead *everstead = Everstead::GetInstance();
    glm::dmat4 lightView = everstead->GetGameRenderer()->GetViewMatrix(everstead->GetCamera());
    lightView[3][0]      = 0.0;
    lightView[3][1]      = 0.0;
    lightView[3][2]      = 0.0;
    Lighting::SetEyeLights(GlStateManager::GetMatrix() * lightView);
    Lighting::SetEntityLight(0xFFFFFFFF);
    Lighting::TurnOn();

    GlStateManager::Rotatef(pitchDelta * 0.1f, 1.0f, 0.0f, 0.0f);
    GlStateManager::Rotatef(yawDelta * 0.1f, 0.0f, 1.0f, 0.0f);

    if (m_selectedItem)
    {
        RenderHeldItem(player, partialTicks, equipHeight, lightColor);
    }
    else
    {
        RenderArm(player, partialTicks, equipHeight, lightColor);
    }

    Lighting::TurnOff();
    Lighting::ClearEyeLights();

    if (player->GetSharedFlag(eEntityFlag_OnFire))
    {
        RenderFireOverlay();
    }

    GlStateManager::EnableDepthTest();
    GlStateManager::DisableBlend();

    GlStateManager::PopMatrix();
    GlStateManager::MatrixMode(1);
    GlStateManager::PopMatrix();
    GlStateManager::MatrixMode(0);
}

void ItemInHandRenderer::RenderArm(LocalPlayer *player, float partialTicks, float equipHeight, uint32_t lightColor)
{
    const float pi = std::numbers::pi_v<float>;
    float swing    = player->GetAttackAnim(partialTicks);
    float swing1   = sinf(swing * pi);
    float swing2   = sinf(sqrtf(swing) * pi);
    float swing3   = sinf(swing * swing * pi);

    GlStateManager::PushMatrix();
    GlStateManager::Translatef(-swing2 * 0.3f, sinf(sqrtf(swing) * pi * 2.0f) * 0.4f, -swing1 * 0.4f);

    GlStateManager::Translatef(0.8f * 0.8f, -0.75f * 0.8f - (1.0f - equipHeight) * 0.6f, -0.9f * 0.8f);
    GlStateManager::Rotatef(45.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(swing2 * 70.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(-swing3 * 20.0f, 0.0f, 0.0f, 1.0f);
    GlStateManager::Translatef(-1.0f, 3.6f, 3.5f);
    GlStateManager::Rotatef(120.0f, 0.0f, 0.0f, 1.0f);
    GlStateManager::Rotatef(200.0f, 1.0f, 0.0f, 0.0f);
    GlStateManager::Rotatef(-135.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Translatef(5.6f, 0.0f, 0.0f);

    GlStateManager::Scalef(1.0f / 16.0f, 1.0f / 16.0f, 1.0f / 16.0f);

    const ModelPartSkin &skin        = m_armModel->GetSkin();
    std::shared_ptr<Texture> texture = ModelRegistry::GetInstance()->GetTextures()->GetPlayerSkin(skin.GetTexturePath());
    Textures::Bind(texture->GetId());
    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.003f);
    m_armModel->RenderPart(m_arm, skin.GetTextureWidth(), skin.GetTextureHeight(), lightColor, glm::dmat4(1.0));
    GlStateManager::SetAlphaCutoff(alphaCutoff);
    Textures::Unbind();

    GlStateManager::PopMatrix();
}

void ItemInHandRenderer::RenderHeldItem(LocalPlayer *player, float partialTicks, float equipHeight, uint32_t lightColor)
{
    const Icon &icon = player->IsDrawingBow() ? player->GetHeldItemIcon() : m_selectedItem->GetIcon();
    if (icon.IsEmpty())
    {
        return;
    }

    ItemAtlas::GetInstance()->GetAtlas()->MarkVisible({icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()});

    const float pi = std::numbers::pi_v<float>;
    float swing    = player->GetAttackAnim(partialTicks);
    float swing1   = sinf(swing * pi);
    float swing2   = sinf(sqrtf(swing) * pi);
    float swing3   = sinf(swing * swing * pi);

    GlStateManager::PushMatrix();
    bool drawingBow = player->IsDrawingBow();
    if (player->IsUsingItem() && !player->IsBlocking() && !drawingBow)
    {
        float t   = (float) player->GetUseItemDuration() - partialTicks + 1.0f;
        float use = std::max(1.0f - t / (float) m_selectedItem->GetUseDuration(), 0.0f);
        float iss = 1.0f - powf(1.0f - use, 27.0f);
        GlStateManager::Translatef(0.0f, fabsf(cosf(t / 4.0f * pi) * 0.1f) * (use > 0.2f ? 1.0f : 0.0f), 0.0f);
        GlStateManager::Translatef(iss * 0.6f, -iss * 0.5f, 0.0f);
        GlStateManager::Rotatef(iss * 90.0f, 0.0f, 1.0f, 0.0f);
        GlStateManager::Rotatef(iss * 10.0f, 1.0f, 0.0f, 0.0f);
        GlStateManager::Rotatef(iss * 30.0f, 0.0f, 0.0f, 1.0f);
    }
    else if (!player->IsUsingItem())
    {
        GlStateManager::Translatef(-swing2 * 0.4f, sinf(sqrtf(swing) * pi * 2.0f) * 0.2f, -swing1 * 0.2f);
    }

    GlStateManager::Translatef(0.7f * 0.8f, -0.65f * 0.8f - (1.0f - equipHeight) * 0.6f, -0.9f * 0.8f);
    GlStateManager::Rotatef(45.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(-swing3 * 20.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(-swing2 * 20.0f, 0.0f, 0.0f, 1.0f);
    GlStateManager::Rotatef(-swing2 * 80.0f, 1.0f, 0.0f, 0.0f);
    GlStateManager::Scalef(0.4f, 0.4f, 0.4f);
    if (player->IsBlocking())
    {
        GlStateManager::Translatef(-0.5f, 0.2f, 0.0f);
        GlStateManager::Rotatef(30.0f, 0.0f, 1.0f, 0.0f);
        GlStateManager::Rotatef(-80.0f, 1.0f, 0.0f, 0.0f);
        GlStateManager::Rotatef(60.0f, 0.0f, 1.0f, 0.0f);
    }

    if (drawingBow)
    {
        GlStateManager::Rotatef(-18.0f, 0.0f, 0.0f, 1.0f);
        GlStateManager::Rotatef(-12.0f, 0.0f, 1.0f, 0.0f);
        GlStateManager::Rotatef(-8.0f, 1.0f, 0.0f, 0.0f);
        GlStateManager::Translatef(-0.9f, 0.2f, 0.0f);
        float t     = (float) player->GetTicksUsingItem() + partialTicks - 1.0f;
        float power = t / 20.0f;
        power       = std::min((power * power + power * 2.0f) / 3.0f, 1.0f);
        if (power > 0.1f)
        {
            GlStateManager::Translatef(0.0f, sinf((t - 0.1f) * 1.3f) * 0.01f * (power - 0.1f), 0.0f);
        }

        GlStateManager::Translatef(0.0f, 0.0f, power * 0.1f);
        GlStateManager::Rotatef(-335.0f, 0.0f, 0.0f, 1.0f);
        GlStateManager::Rotatef(-50.0f, 0.0f, 1.0f, 0.0f);
        GlStateManager::Translatef(0.0f, 0.5f, 0.0f);
        GlStateManager::Scalef(1.0f, 1.0f, 1.0f + power * 0.2f);
        GlStateManager::Translatef(0.0f, -0.5f, 0.0f);
        GlStateManager::Rotatef(50.0f, 0.0f, 1.0f, 0.0f);
        GlStateManager::Rotatef(335.0f, 0.0f, 0.0f, 1.0f);
    }

    Block *heldBlock = m_selectedItem->GetBlock();
    if (heldBlock && heldBlock->RendersAsItemBlock() && heldBlock->GetRenderShape() == Block::eRenderShape_BOXES)
    {
        std::vector<AABB> boxes;
        heldBlock->GetItemBoxes(&boxes);
        double top = 0.0;
        for (size_t i = 0; i < boxes.size(); i++)
        {
            top = std::max(top, boxes[i].GetMax().y);
        }

        if (!boxes.empty() && top <= s_thinBlockHeight)
        {
            GlStateManager::Translatef(0.0f, (float) (1.0 - top), 0.0f);
        }
    }

    GlStateManager::ClearDepth();
    GlStateManager::EnableDepthTest();
    GlStateManager::SetGlint(FractureTraits::GetGlint(*player->GetInventory()->GetSelectedItem()));
    RenderItem(m_selectedItem, icon, lightColor);
    GlStateManager::SetGlint(0.0f);
    GlStateManager::DisableDepthTest();

    GlStateManager::PopMatrix();
}

void ItemInHandRenderer::RenderItem(Item *item, const Icon &icon, uint32_t lightColor)
{
    if (!item)
    {
        return;
    }

    Block *block = item->GetBlock();
    if (block && block->RendersAsItemBlock())
    {
        GlStateManager::PushMatrix();
        BlockRenderer::RenderBlockAsItem(block, lightColor);
        GlStateManager::PopMatrix();
        return;
    }

    if (icon.IsEmpty())
    {
        return;
    }

    ItemAtlas::GetInstance()->GetAtlas()->MarkVisible({icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()});

    GlStateManager::PushMatrix();
    GlStateManager::Translatef(0.0f, -0.3f, 0.0f);
    GlStateManager::Scalef(1.5f, 1.5f, 1.5f);
    GlStateManager::Rotatef(50.0f, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(335.0f, 0.0f, 0.0f, 1.0f);
    GlStateManager::Translatef(-15.0f / 16.0f, -1.0f / 16.0f, 0.0f);

    uint32_t spriteColor = lightColor;
    if (block && block->HasBiomeTint(Direction::north))
    {
        uint32_t tint = block->GetItemTint();
        uint32_t r    = ((lightColor >> 16) & 0xFF) * ((tint >> 16) & 0xFF) / 255;
        uint32_t g    = ((lightColor >> 8) & 0xFF) * ((tint >> 8) & 0xFF) / 255;
        uint32_t b    = (lightColor & 0xFF) * (tint & 0xFF) / 255;
        spriteColor   = (lightColor & 0xFF000000) | (r << 16) | (g << 8) | b;
    }

    RenderItemSprite(icon, spriteColor);

    GlStateManager::PopMatrix();
}

// This renders the item centered on the origin, blocks as a unit cube and sprites as a unit square
void ItemInHandRenderer::RenderDroppedItem(Item *item, uint32_t lightColor, bool flat)
{
    if (!item)
    {
        return;
    }

    Block *block = item->GetBlock();
    if (block && block->RendersAsItemBlock())
    {
        GlStateManager::PushMatrix();
        BlockRenderer::RenderBlockAsItem(block, lightColor);
        GlStateManager::PopMatrix();
        return;
    }

    const Icon &icon = item->GetIcon();
    if (icon.IsEmpty())
    {
        return;
    }

    ItemAtlas::GetInstance()->GetAtlas()->MarkVisible({icon.GetU0(), icon.GetV0(), icon.GetU1(), icon.GetV1()});

    GlStateManager::PushMatrix();
    GlStateManager::Translatef(-0.5f, -0.5f, flat ? 0.0f : s_spriteDepth * 0.5f);

    uint32_t spriteColor = lightColor;
    if (block && block->HasBiomeTint(Direction::north))
    {
        uint32_t tint = block->GetItemTint();
        uint32_t r    = ((lightColor >> 16) & 0xFF) * ((tint >> 16) & 0xFF) / 255;
        uint32_t g    = ((lightColor >> 8) & 0xFF) * ((tint >> 8) & 0xFF) / 255;
        uint32_t b    = (lightColor & 0xFF) * (tint & 0xFF) / 255;
        spriteColor   = (lightColor & 0xFF000000) | (r << 16) | (g << 8) | b;
    }

    if (flat)
    {
        GlStateManager::DisableCull();
        float alphaCutoff = GlStateManager::GetAlphaCutoff();
        GlStateManager::SetAlphaCutoff(0.1f);
        Textures::Bind(icon.GetTexture()->GetId());
        BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(spriteColor);
        builder->VertexUV(0.0f, 0.0f, 0.0f, icon.GetU1(), icon.GetV1());
        builder->VertexUV(1.0f, 0.0f, 0.0f, icon.GetU0(), icon.GetV1());
        builder->VertexUV(1.0f, 1.0f, 0.0f, icon.GetU0(), icon.GetV0());
        builder->VertexUV(0.0f, 0.0f, 0.0f, icon.GetU1(), icon.GetV1());
        builder->VertexUV(1.0f, 1.0f, 0.0f, icon.GetU0(), icon.GetV0());
        builder->VertexUV(0.0f, 1.0f, 0.0f, icon.GetU1(), icon.GetV0());
        builder->End();
        GlStateManager::SetAlphaCutoff(alphaCutoff);
        GlStateManager::EnableCull();
    }
    else
    {
        RenderItemSprite(icon, spriteColor);
    }

    GlStateManager::PopMatrix();
}

void ItemInHandRenderer::EmitSpriteQuad(std::vector<BufferBuilder::Vtx> *vertices, const Icon &icon, float brightness, const float corners[4][5])
{
    const int order[6] = {0, 1, 2, 0, 2, 3};
    for (int index : order)
    {
        const float *corner = corners[index];
        float u             = icon.GetU1() - corner[3] * (icon.GetU1() - icon.GetU0());
        float v             = icon.GetV1() - corner[4] * (icon.GetV1() - icon.GetV0());
        vertices->push_back({corner[0], corner[1], corner[2], u, v, brightness, brightness, brightness, 1.0f});
    }
}

// Faces are merged into runs and cached per icon, the front and back are one quad each and rely on the alpha cutoff
const std::vector<BufferBuilder::Vtx> &ItemInHandRenderer::GetSpriteMesh(const Icon &icon)
{
    std::tuple<const Texture *, float, float> key(icon.GetTexture(), icon.GetU0(), icon.GetV0());
    auto it = s_spriteMeshes.find(key);
    if (it != s_spriteMeshes.end())
    {
        return it->second;
    }

    std::vector<BufferBuilder::Vtx> &vertices = s_spriteMeshes[key];

    float texelU                                                = (icon.GetU1() - icon.GetU0()) / (float) s_spriteResolution;
    float texelV                                                = (icon.GetV1() - icon.GetV0()) / (float) s_spriteResolution;
    const Texture *texture                                      = icon.GetTexture();
    const std::vector<uint8_t> &pixels                          = texture->GetPixels();
    int width                                                   = texture->GetPixelWidth();
    int height                                                  = texture->GetPixelHeight();
    bool filled[s_spriteResolution + 2][s_spriteResolution + 2] = {};
    for (int yp = 0; yp < s_spriteResolution; yp++)
    {
        for (int xp = 0; xp < s_spriteResolution; xp++)
        {
            float u                = icon.GetU0() + ((float) (s_spriteResolution - 1 - xp) + 0.5f) * texelU;
            float v                = icon.GetV0() + ((float) (s_spriteResolution - 1 - yp) + 0.5f) * texelV;
            int px                 = std::min(std::max((int) (u * (float) width), 0), width - 1);
            int py                 = std::min(std::max((int) (v * (float) height), 0), height - 1);
            size_t offset          = ((size_t) py * (size_t) width + (size_t) px) * 4 + 3;
            filled[yp + 1][xp + 1] = offset < pixels.size() && pixels[offset] != 0;
        }
    }

    float z0          = 0.0f;
    float z1          = -s_spriteDepth;
    float front[4][5] = {{0, 0, z0, 0, 0}, {1, 0, z0, 1, 0}, {1, 1, z0, 1, 1}, {0, 1, z0, 0, 1}};
    float back[4][5]  = {{0, 1, z1, 0, 1}, {1, 1, z1, 1, 1}, {1, 0, z1, 1, 0}, {0, 0, z1, 0, 0}};
    EmitSpriteQuad(&vertices, icon, 0.8f, front);
    EmitSpriteQuad(&vertices, icon, 0.8f, back);

    float step = 1.0f / (float) s_spriteResolution;
    for (int line = 0; line < s_spriteResolution; line++)
    {
        float lineMin    = (float) line * step;
        float lineMax    = lineMin + step;
        float lineCenter = lineMin + step * 0.5f;

        // Each side scans for runs of exposed pixels along the line, 0 left, 1 right, 2 bottom, 3 top
        for (int side = 0; side < 4; side++)
        {
            int start = -1;
            for (int along = 0; along <= s_spriteResolution; along++)
            {
                bool exposed = false;
                if (along < s_spriteResolution)
                {
                    int xp  = side < 2 ? line : along;
                    int yp  = side < 2 ? along : line;
                    int nx  = xp + (side == 0 ? -1 : side == 1 ? 1 : 0);
                    int ny  = yp + (side == 2 ? -1 : side == 3 ? 1 : 0);
                    exposed = filled[yp + 1][xp + 1] && !filled[ny + 1][nx + 1];
                }

                if (exposed && start < 0)
                {
                    start = along;
                }
                if (exposed || start < 0)
                {
                    continue;
                }

                float a0 = (float) start * step;
                float a1 = (float) along * step;
                start    = -1;
                if (side == 0)
                {
                    float quad[4][5] = {{lineMin, a0, z1, lineCenter, a0},
                                        {lineMin, a0, z0, lineCenter, a0},
                                        {lineMin, a1, z0, lineCenter, a1},
                                        {lineMin, a1, z1, lineCenter, a1}};
                    EmitSpriteQuad(&vertices, icon, 0.6f, quad);
                }
                else if (side == 1)
                {
                    float quad[4][5] = {{lineMax, a1, z1, lineCenter, a1},
                                        {lineMax, a1, z0, lineCenter, a1},
                                        {lineMax, a0, z0, lineCenter, a0},
                                        {lineMax, a0, z1, lineCenter, a0}};
                    EmitSpriteQuad(&vertices, icon, 0.6f, quad);
                }
                else if (side == 2)
                {
                    float quad[4][5] = {{a1, lineMin, z0, a1, lineCenter},
                                        {a0, lineMin, z0, a0, lineCenter},
                                        {a0, lineMin, z1, a0, lineCenter},
                                        {a1, lineMin, z1, a1, lineCenter}};
                    EmitSpriteQuad(&vertices, icon, 1.0f, quad);
                }
                else
                {
                    float quad[4][5] = {{a1, lineMax, z1, a1, lineCenter},
                                        {a0, lineMax, z1, a0, lineCenter},
                                        {a0, lineMax, z0, a0, lineCenter},
                                        {a1, lineMax, z0, a1, lineCenter}};
                    EmitSpriteQuad(&vertices, icon, 0.5f, quad);
                }
            }
        }
    }

    return vertices;
}

void ItemInHandRenderer::RenderItemSprite(const Icon &icon, uint32_t lightColor)
{
    const std::vector<BufferBuilder::Vtx> &vertices = GetSpriteMesh(icon);
    BufferBuilder *builder                          = Tesselator::GetInstance()->GetBuilder();

    GlStateManager::DisableCull();

    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.003f);
    Textures::Bind(icon.GetTexture()->GetId());

    float a = (float) ((lightColor >> 24) & 0xFF) / 255.0f;
    float r = (float) ((lightColor >> 16) & 0xFF) / 255.0f;
    float g = (float) ((lightColor >> 8) & 0xFF) / 255.0f;
    float b = (float) (lightColor & 0xFF) / 255.0f;

    bool eyeLights = Lighting::HasEyeLights();
    // We draw a depth-only pass first so each pixel shows only the nearest sprite surface, otherwise translucent pixels stack with the faces behind them
    for (int pass = 0; pass < 2; pass++)
    {
        bool colorPass = pass == 1;
        GlStateManager::SetColorMask(colorPass, colorPass, colorPass, colorPass);
        builder->Begin(ePrimitiveType_Triangles);
        for (size_t i = 0; i + 2 < vertices.size(); i += 3)
        {
            float shade[3] = {vertices[i].r, vertices[i].g, vertices[i].b};
            if (eyeLights)
            {
                const BufferBuilder::Vtx &v0 = vertices[i];
                const BufferBuilder::Vtx &v1 = vertices[i + 1];
                const BufferBuilder::Vtx &v2 = vertices[i + 2];
                float ex                     = v1.x - v0.x;
                float ey                     = v1.y - v0.y;
                float ez                     = v1.z - v0.z;
                float fx                     = v2.x - v0.x;
                float fy                     = v2.y - v0.y;
                float fz                     = v2.z - v0.z;
                float diffuse                = Lighting::GetEyeDiffuse(ey * fz - ez * fy, ez * fx - ex * fz, ex * fy - ey * fx);
                shade[0]                     = diffuse;
                shade[1]                     = diffuse;
                shade[2]                     = diffuse;
            }

            for (size_t j = i; j < i + 3; j++)
            {
                const BufferBuilder::Vtx &vertex = vertices[j];
                builder->Color(r * shade[0], g * shade[1], b * shade[2], a);
                builder->VertexUV(vertex.x, vertex.y, vertex.z, vertex.u, vertex.v);
            }
        }

        builder->End();
    }

    Textures::Unbind();
    GlStateManager::SetAlphaCutoff(alphaCutoff);

    GlStateManager::EnableCull();
}

void ItemInHandRenderer::EnsureArmModel(LocalPlayer *player)
{
    if (m_armModel && m_armTexturePath == player->GetSkinTexturePath() && m_armSkinModel == player->GetSkinModel())
    {
        return;
    }

    delete m_armModel;
    m_armModel       = nullptr;
    m_arm            = nullptr;
    m_armTexturePath = player->GetSkinTexturePath();
    m_armSkinModel   = player->GetSkinModel();

    double rightArmMin = -3.0;
    double rightArmMax = 1.0;
    int rightArmRightX = 52;
    int rightArmFrontX = 56;
    int rightArmBackX  = 48;
    int rightArmDownX  = 48;
    int frontWidth     = -4;
    int topWidth       = 4;
    if (m_armSkinModel == ePlayerSkinModel_Slim)
    {
        rightArmMin    = -2.0;
        rightArmMax    = 1.0;
        rightArmRightX = 51;
        rightArmFrontX = 54;
        rightArmBackX  = 47;
        rightArmDownX  = 47;
        frontWidth     = -3;
        topWidth       = 3;
    }

    ModelPartCubeDefinition::FaceUV uv;
    uv.left   = {44, 32, -4, -12};
    uv.right  = {rightArmRightX, 32, -4, -12};
    uv.front  = {rightArmFrontX, 32, frontWidth, -12};
    uv.back   = {rightArmBackX, 32, frontWidth, -12};
    uv.top    = {rightArmDownX, 16, topWidth, 4};
    uv.bottom = {44, 16, topWidth, 4};

    ModelPartCubeDefinition::FaceUV sleeveUv;
    sleeveUv.left   = {44, 48, -4, -12};
    sleeveUv.right  = {rightArmRightX, 48, -4, -12};
    sleeveUv.front  = {rightArmFrontX, 48, frontWidth, -12};
    sleeveUv.back   = {rightArmBackX, 48, frontWidth, -12};
    sleeveUv.top    = {rightArmDownX, 32, topWidth, 4};
    sleeveUv.bottom = {44, 32, topWidth, 4};

    ModelDefinition definition("firstPersonHand", ModelPartSkin(m_armTexturePath, 64, 64));
    std::unique_ptr<ModelPartDefinition> rightArm = std::make_unique<ModelPartDefinition>("rightArm");
    rightArm->SetPosition(glm::dvec3(-5.0, 2.0, 0.0));
    rightArm->AddCube(ModelPartCubeDefinition("right_arm_cube", glm::dvec3(rightArmMin, -2.0, -2.0), glm::dvec3(rightArmMax, 10.0, 2.0), uv, 0.0f));
    rightArm->AddCube(
            ModelPartCubeDefinition("right_sleeve_cube", glm::dvec3(rightArmMin, -2.0, -2.0), glm::dvec3(rightArmMax, 10.0, 2.0), sleeveUv, 0.25f));
    definition.GetRoot()->AddChild(std::move(rightArm));

    m_armModel = new Model(definition);
    m_arm      = m_armModel->FindPart("rightArm");
}

void ItemInHandRenderer::RenderFireOverlay()
{
    Block *fire      = Block::ByName("fire");
    Block::UVRect uv = fire->GetSecondaryAtlasUVRect();
    float v0         = uv.v0;
    float v1         = uv.v1;

    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::DisableCull();
    Textures::Bind(fire->GetTexture(Direction::up)->GetId());

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    for (int i = 0; i < 2; i++)
    {
        float side = (float) (i * 2 - 1);
        GlStateManager::PushMatrix();
        GlStateManager::Translatef(-side * 0.24f, -0.3f, 0.0f);
        GlStateManager::Rotatef(side * 10.0f, 0.0f, 1.0f, 0.0f);

        float u0 = i == 0 ? uv.u0 : uv.u1;
        float u1 = i == 0 ? uv.u1 : uv.u0;
        builder->Begin(ePrimitiveType_Triangles);
        builder->Color(0xE6FFFFFF);
        builder->VertexUV(-0.5f, -0.5f, -0.5f, u1, v1);
        builder->VertexUV(0.5f, -0.5f, -0.5f, u0, v1);
        builder->VertexUV(0.5f, 0.5f, -0.5f, u0, v0);
        builder->VertexUV(-0.5f, -0.5f, -0.5f, u1, v1);
        builder->VertexUV(0.5f, 0.5f, -0.5f, u0, v0);
        builder->VertexUV(-0.5f, 0.5f, -0.5f, u1, v0);
        builder->End();

        GlStateManager::PopMatrix();
    }

    Textures::Unbind();
    GlStateManager::EnableCull();
    GlStateManager::PopMatrix();
}

#endif
