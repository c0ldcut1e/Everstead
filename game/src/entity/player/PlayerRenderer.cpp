#ifndef EVERSTEAD_DEDICATED_SERVER

#include "PlayerRenderer.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Textures.h"
#include "../../scene/Camera.h"
#include "../../utils/Direction.h"
#include "../../world/ItemInHandRenderer.h"
#include "../../world/Level.h"
#include "../../world/block/BedBlock.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRenderer.h"
#include "../../world/fracture/FractureTraits.h"
#include "../../world/item/ArmorItem.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../../world/lighting/Lighting.h"
#include "../../world/models/Model.h"
#include "../../world/models/ModelPartsSkinned.h"
#include "../../world/models/ModelRegistry.h"
#include "RemotePlayer.h"

#include <glm/trigonometric.hpp>

const float PlayerRenderer::s_heldItemArmOffsetX = -1.0f;
const float PlayerRenderer::s_heldItemArmOffsetY = -7.0f;
const float PlayerRenderer::s_heldItemArmOffsetZ = -1.0f;
const float PlayerRenderer::s_heldItemDropY      = -3.0f;
const float PlayerRenderer::s_heldItemScale      = 10.0f;
const float PlayerRenderer::s_heldBlockOffsetY   = -3.0f;
const float PlayerRenderer::s_heldBlockOffsetZ   = 5.0f;
const float PlayerRenderer::s_heldBlockScale     = 6.0f;
const float PlayerRenderer::s_heldArmPitch       = std::numbers::pi_v<float> * 0.5f * 0.2f;

PlayerRenderer::PlayerRenderer()
{
    m_inGui = false;
}

Model *PlayerRenderer::GetSkinModel(const std::string &texturePath, ePLAYER_SKIN_MODEL skinModel) const
{
    std::unique_ptr<Model> &model = m_skinModels[texturePath + "#" + std::to_string((int) skinModel)];
    if (!model)
    {
        model.reset(new Model(ModelPartsSkinned::CreatePlayer("player", texturePath, skinModel)));
    }

    return model.get();
}

Model *PlayerRenderer::GetCapeModel(const std::string &texturePath) const
{
    std::unique_ptr<Model> &model = m_capeModels[texturePath];
    if (!model)
    {
        model.reset(new Model(ModelPartsSkinned::CreateCape("cape", texturePath)));
    }

    return model.get();
}

void PlayerRenderer::Render(const Entity *entity, float partialTicks, uint32_t lightColor) const
{
    const RemotePlayer *player = (const RemotePlayer *) entity;
    if (player->GetDeathTime() >= 20)
    {
        return;
    }

    glm::dvec3 pos = entity->GetRenderPosition(partialTicks);

    const float steveMinYPx   = -2.0f;
    const float steveHeightPx = 32.0f;

    const AABB &box    = entity->GetAABB();
    float entityHeight = box.GetMax().y - box.GetMin().y;
    float finalScale   = (1.0f / 16.0f) * (entityHeight / (steveHeightPx / 16.0f));
    float bodyYaw      = player->GetRenderBodyYaw(partialTicks);
    bool sneaking      = player->IsSneaking();
    float yawRotation  = 90.0f - bodyYaw;
    float lieRotation  = 0.0f;
    BedBlock *bed      = player->IsSleeping() ? dynamic_cast<BedBlock *>(Block::ById(entity->GetLevel()->GetBlockId(player->GetBedPos()))) : nullptr;
    if (bed)
    {
        // We tip the model onto its back with local -Z pointing at the pillow, so the feet sit one block behind the position
        Direction *head = bed->GetHeadDirection(entity->GetLevel()->GetBlockData(player->GetBedPos()));
        yawRotation     = glm::degrees(atan2f((float) -head->dx, (float) -head->dz));
        lieRotation     = -90.0f;
        pos += glm::dvec3(-head->dx, 0.0, -head->dz);
        sneaking = false;
    }

    glm::dmat4 modelMatrix = glm::dmat4(1.0);
    modelMatrix            = glm::translate(modelMatrix, pos);
    modelMatrix            = glm::rotate(modelMatrix, yawRotation * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
    modelMatrix            = glm::rotate(modelMatrix, lieRotation * std::numbers::pi / 180.0, glm::dvec3(1.0, 0.0, 0.0));
    if (player->GetDeathTime() > 0)
    {
        float fall  = std::min(1.0f, sqrtf(((float) player->GetDeathTime() + partialTicks - 1.0f) / 20.0f * 1.6f));
        modelMatrix = glm::rotate(modelMatrix, -fall * std::numbers::pi / 2.0, glm::dvec3(0.0, 0.0, 1.0));
    }

    modelMatrix = glm::scale(modelMatrix, glm::dvec3(finalScale, finalScale, finalScale));
    modelMatrix = glm::translate(modelMatrix, glm::dvec3(0.0, -steveMinYPx, 0.0));

    if (sneaking)
    {
        modelMatrix = glm::translate(modelMatrix, glm::dvec3(0.0, -3.0, 0.0));
    }

    GlStateManager::PushMatrix();

    GlStateManager::Translated(pos.x, pos.y, pos.z);

    GlStateManager::Rotatef(yawRotation, 0.0f, 1.0f, 0.0f);
    GlStateManager::Rotatef(lieRotation, 1.0f, 0.0f, 0.0f);
    if (player->GetDeathTime() > 0)
    {
        float fall = std::min(1.0f, sqrtf(((float) player->GetDeathTime() + partialTicks - 1.0f) / 20.0f * 1.6f));
        GlStateManager::Rotatef(-fall * 90.0f, 0.0f, 0.0f, 1.0f);
    }

    GlStateManager::Scalef(finalScale, finalScale, finalScale);
    GlStateManager::Translatef(0.0f, -steveMinYPx, 0.0f);

    if (sneaking)
    {
        GlStateManager::Translatef(0.0f, -3.0f, 0.0f);
    }

    Model *model     = GetSkinModel(player->GetSkinTexturePath(), player->GetSkinModel());
    float limbSwing  = player->GetLimbSwingOld() + (player->GetLimbSwing() - player->GetLimbSwingOld()) * partialTicks;
    float limbAmount = player->GetLimbSwingAmountOld() + (player->GetLimbSwingAmount() - player->GetLimbSwingAmountOld()) * partialTicks;

    const float armScale = 2.0f;
    const float legScale = 1.4f;
    float phase          = limbSwing * 0.6662f;

    float rightArmX = cosf(phase + std::numbers::pi_v<float>) * armScale * limbAmount * 0.5f;
    float leftArmX  = cosf(phase) * armScale * limbAmount * 0.5f;
    float rightLegX = cosf(phase) * legScale * limbAmount;
    float leftLegX  = cosf(phase + std::numbers::pi_v<float>) * legScale * limbAmount;
    float legSpread = 0.0f;
    float legRoll   = 0.0f;
    if (player->IsRiding())
    {
        rightArmX -= std::numbers::pi_v<float> / 5.0f;
        leftArmX -= std::numbers::pi_v<float> / 5.0f;
        rightLegX = -1.4137167f;
        leftLegX  = -1.4137167f;
        legSpread = glm::degrees(std::numbers::pi_v<float> / 10.0f);
        legRoll   = glm::degrees(0.07853982f);
    }

    float bob      = player->GetRenderBob(partialTicks);
    float armSwayZ = cosf(bob * 0.09f) * 0.05f + 0.05f;
    float armSwayX = sinf(bob * 0.067f) * 0.05f;

    float sneakBodyTilt = sneaking ? glm::degrees(0.5f) : 0.0f;
    float sneakArmTilt  = sneaking ? glm::degrees(0.4f) : 0.0f;

    const float pi   = std::numbers::pi_v<float>;
    float attackTime = player->GetAttackAnim(partialTicks);

    float swingBodyYaw   = -sinf(sqrtf(attackTime) * pi * 2.0f) * 0.2f;
    float swingRaiseArg  = 1.0f - attackTime;
    swingRaiseArg        = swingRaiseArg * swingRaiseArg;
    swingRaiseArg        = swingRaiseArg * swingRaiseArg;
    swingRaiseArg        = 1.0f - swingRaiseArg;
    float swingRaise     = sinf(swingRaiseArg * pi);
    float headPitch      = -entity->GetRenderPitch(partialTicks);
    float headPitchRad   = headPitch * (pi / 180.0f);
    float swingHeadBlend = sinf(attackTime * pi) * -(headPitchRad - 0.7f) * 0.75f;
    float swingArmZ      = sinf(attackTime * pi) * 0.4f;

    ModelPart *body         = model->FindPart("body");
    glm::dvec3 bodyRotation = body->GetRotation();
    bodyRotation.x          = sneakBodyTilt;
    bodyRotation.y          = glm::degrees(swingBodyYaw);
    body->SetRotation(bodyRotation);

    ModelPart *head         = model->FindPart("head");
    glm::dvec3 headRotation = head->GetRotation();
    headRotation.x          = bed ? 0.0f : headPitch;
    headRotation.y          = bed ? 0.0f : bodyYaw - player->GetRenderHeadYaw(partialTicks);
    head->SetRotation(headRotation);
    head->SetPosition(glm::dvec3(0.0, sneaking ? 21.0 : 22.0, 0.0));

    ItemInstance *heldItem = ((Player *) player)->GetInventory()->GetSelectedItem();
    bool holdingItem       = heldItem && !heldItem->IsEmpty();

    float rightArmBaseX = rightArmX;
    bool blocking       = ((Player *) player)->IsBlocking();
    bool drawingBow     = ((Player *) player)->IsDrawingBow();
    if (holdingItem)
    {
        rightArmBaseX = rightArmBaseX * 0.5f - s_heldArmPitch * (blocking ? 3.0f : 1.0f);
    }

    ModelPart *rightArm         = model->FindPart("rightArm");
    glm::dvec3 rightArmRotation = rightArm->GetRotation();
    rightArmRotation.x          = glm::degrees(rightArmBaseX + armSwayX - (swingRaise * 1.2f + swingHeadBlend)) + sneakArmTilt;
    rightArmRotation.y          = glm::degrees(swingBodyYaw * 3.0f);
    rightArmRotation.z          = glm::degrees(-armSwayZ + swingArmZ);
    if (drawingBow)
    {
        rightArmRotation = glm::dvec3(headPitch - 90.0f, glm::degrees(0.1f) + headRotation.y, 0.0);
    }
    else if (((Player *) player)->IsUsingItem() && !blocking)
    {
        float t            = (float) ((Player *) player)->GetUseItemDuration() - partialTicks + 1.0f;
        float use          = 1.0f - t / (float) heldItem->GetItem()->GetUseDuration();
        float iss          = 1.0f - powf(1.0f - use, 27.0f);
        float eatX         = -fabsf(cosf(t / 4.0f * pi) * 0.1f) * (use > 0.2f ? 1.0f : 0.0f) * 2.0f - iss * 1.2f;
        rightArmRotation.x = glm::degrees(eatX) + sneakArmTilt;
        rightArmRotation.y += glm::degrees(iss * 0.5f);
    }

    rightArm->SetRotation(rightArmRotation);
    rightArm->SetPosition(glm::dvec3(-cosf(swingBodyYaw) * 5.0f, 20.0, sinf(swingBodyYaw) * 5.0f));

    ModelPart *leftArm         = model->FindPart("leftArm");
    glm::dvec3 leftArmRotation = leftArm->GetRotation();
    leftArmRotation.x          = glm::degrees(leftArmX - armSwayX + swingBodyYaw) + sneakArmTilt;
    leftArmRotation.y          = glm::degrees(swingBodyYaw);
    leftArmRotation.z          = glm::degrees(armSwayZ);
    if (drawingBow)
    {
        leftArmRotation = glm::dvec3(headPitch - 90.0f, glm::degrees(-0.5f) + headRotation.y, 0.0);
    }

    leftArm->SetRotation(leftArmRotation);
    leftArm->SetPosition(glm::dvec3(cosf(swingBodyYaw) * 5.0f, 20.0, -sinf(swingBodyYaw) * 5.0f));

    ModelPart *rightLeg = model->FindPart("rightLeg");
    rightLeg->SetRotation(glm::dvec3(glm::degrees(rightLegX), -legSpread, -legRoll));
    rightLeg->SetPosition(glm::dvec3(-1.9, sneaking ? 13.0 : 10.0, sneaking ? -4.0 : 0.0));

    ModelPart *leftLeg = model->FindPart("leftLeg");
    leftLeg->SetRotation(glm::dvec3(glm::degrees(leftLegX), legSpread, legRoll));
    leftLeg->SetPosition(glm::dvec3(1.9, sneaking ? 13.0 : 10.0, sneaking ? -4.0 : 0.0));

    float alphaCutoff = GlStateManager::GetAlphaCutoff();
    GlStateManager::SetAlphaCutoff(0.003f);
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    uint32_t modelColor = Lighting::IsEnabled() ? 0xFFFFFFFF : lightColor;
    bool spectator      = !m_inGui && player->GetSharedFlag(eEntityFlag_Spectator);
    if (spectator)
    {
        model->Render(*ModelRegistry::GetInstance()->GetTextures(), (modelColor & 0x00FFFFFF) | s_spectatorAlpha, modelMatrix, head);
    }
    else
    {
        if (player->GetHurtTime() > 0 || player->GetDeathTime() > 0)
        {
            GlStateManager::SetOverlayColor(0x66FF0000);
        }

        model->Render(*ModelRegistry::GetInstance()->GetTextures(), modelColor, modelMatrix);

        constexpr const char *kArmorParts[Inventory::s_armorSize][3] = {{"head", nullptr, nullptr},
                                                                        {"body", "rightArm", "leftArm"},
                                                                        {"body", "rightLeg", "leftLeg"},
                                                                        {"rightLeg", "leftLeg", nullptr}};
        Inventory *inventory                                         = ((Player *) player)->GetInventory();
        for (int i = 0; i < Inventory::s_armorSize; i++)
        {
            const ItemInstance *armor = inventory->GetArmor(i);
            if (armor->IsEmpty() || armor->GetItem()->GetArmorSlot() < 0)
            {
                continue;
            }

            Model *armorModel = ModelRegistry::GetInstance()->GetModel(i == ArmorItem::s_legs ? "armor_inner" : "armor_outer");
            Textures::Bind(ModelRegistry::GetInstance()->GetTextures()->Get(((ArmorItem *) armor->GetItem())->GetLayerTexture())->GetId());
            GlStateManager::SetGlint(FractureTraits::GetGlint(*armor));
            for (int j = 0; j < 3 && kArmorParts[i][j]; j++)
            {
                ModelPart *source = model->FindPart(kArmorParts[i][j]);
                ModelPart *part   = armorModel->FindPart(kArmorParts[i][j]);
                part->SetRotation(source->GetRotation());
                part->SetPosition(source->GetPosition());
                armorModel->RenderPart(part, 64, 32, modelColor, modelMatrix);
            }
        }

        GlStateManager::SetGlint(0.0f);
        Textures::Unbind();
        GlStateManager::SetOverlayColor(0);
    }

    if (!player->GetCapeTexturePath().empty() && !spectator)
    {
        Model *capeModel        = GetCapeModel(player->GetCapeTexturePath());
        glm::dvec3 capePosition = player->GetCapePosition(partialTicks);
        double xd               = capePosition.x - pos.x;
        double yd               = capePosition.y - pos.y;
        double zd               = capePosition.z - pos.z;
        double yawRadians       = (double) bodyYaw * std::numbers::pi / 180.0;
        double xa               = -cos(yawRadians);
        double za               = -sin(yawRadians);
        float flap              = (float) yd * 10.0f;
        if (flap < -6.0f)
        {
            flap = -6.0f;
        }

        if (flap > 32.0f)
        {
            flap = 32.0f;
        }

        float lean  = (float) (xd * xa + zd * za) * 100.0f;
        float lean2 = (float) (xd * za - zd * xa) * 100.0f;
        if (lean < 0.0f)
        {
            lean = 0.0f;
        }

        float capeWalkDist = player->GetWalkDistOld() + (player->GetWalkDist() - player->GetWalkDistOld()) * partialTicks;
        float capeBob      = player->GetWalkBobOld() + (player->GetWalkBob() - player->GetWalkBobOld()) * partialTicks;
        flap += sinf(capeWalkDist * 6.0f) * 32.0f * capeBob;
        if (sneaking)
        {
            flap += 25.0f;
        }

        float xRot = 6.0f + lean * 0.5f + flap;
        if (xRot > 64.0f)
        {
            xRot = 64.0f;
        }

        ModelPart *cape = capeModel->FindPart("cape");
        cape->SetRotation(glm::dvec3(0.0, 0.0, 0.0));
        glm::dvec3 capePivot = cape->GetPosition();
        if (!((Player *) player)->GetInventory()->GetArmor(ArmorItem::s_torso)->IsEmpty())
        {
            capePivot += glm::dvec3(0.0, 0.85, -1.1);
        }

        GlStateManager::PushMatrix();
        GlStateManager::Translatef((float) capePivot.x, (float) capePivot.y, (float) capePivot.z);
        GlStateManager::Rotatef(xRot, 1.0f, 0.0f, 0.0f);
        GlStateManager::Rotatef(-lean2 * 0.5f, 0.0f, 0.0f, 1.0f);
        GlStateManager::Rotatef(lean2 * 0.5f, 0.0f, 1.0f, 0.0f);
        GlStateManager::Translatef(-(float) cape->GetPosition().x, -(float) cape->GetPosition().y, -(float) cape->GetPosition().z);
        glm::dmat4 capeMatrix = modelMatrix;
        capeMatrix            = glm::translate(capeMatrix, capePivot);
        capeMatrix            = glm::rotate(capeMatrix, xRot * std::numbers::pi / 180.0, glm::dvec3(1.0, 0.0, 0.0));
        capeMatrix            = glm::rotate(capeMatrix, -lean2 * 0.5f * std::numbers::pi / 180.0, glm::dvec3(0.0, 0.0, 1.0));
        capeMatrix            = glm::rotate(capeMatrix, lean2 * 0.5f * std::numbers::pi / 180.0, glm::dvec3(0.0, 1.0, 0.0));
        capeMatrix            = glm::translate(capeMatrix, -cape->GetPosition());
        capeModel->Render(*ModelRegistry::GetInstance()->GetTextures(), modelColor, capeMatrix);

        GlStateManager::PopMatrix();
    }

    if (holdingItem && !spectator)
    {
        const glm::dvec3 &armPosition = rightArm->GetPosition();
        const glm::dvec3 &armPivot    = rightArm->GetPivot();
        const glm::dvec3 &armRotation = rightArm->GetRotation();

        GlStateManager::PushMatrix();
        GlStateManager::Translatef((float) armPosition.x, (float) armPosition.y, (float) armPosition.z);
        GlStateManager::Translatef((float) armPivot.x, (float) armPivot.y, (float) armPivot.z);
        GlStateManager::Rotatef((float) armRotation.z, 0.0f, 0.0f, 1.0f);
        GlStateManager::Rotatef((float) armRotation.y, 0.0f, 1.0f, 0.0f);
        GlStateManager::Rotatef((float) armRotation.x, 1.0f, 0.0f, 0.0f);
        GlStateManager::Translatef((float) -armPivot.x, (float) -armPivot.y, (float) -armPivot.z);

        GlStateManager::Translatef(s_heldItemArmOffsetX, s_heldItemArmOffsetY, s_heldItemArmOffsetZ);

        GlStateManager::SetGlint(FractureTraits::GetGlint(*heldItem));
        Block *heldBlock = heldItem->GetItem() ? heldItem->GetItem()->GetBlock() : nullptr;
        if (heldBlock && heldBlock->RendersAsItemBlock())
        {
            GlStateManager::Translatef(0.0f, s_heldBlockOffsetY, s_heldBlockOffsetZ);
            GlStateManager::Rotatef(20.0f, 1.0f, 0.0f, 0.0f);
            GlStateManager::Rotatef(-45.0f, 0.0f, 1.0f, 0.0f);
            GlStateManager::Scalef(s_heldBlockScale, s_heldBlockScale, s_heldBlockScale);

            BlockRenderer::RenderBlockAsItem(heldBlock, lightColor);
        }
        else if (heldItem->GetItem()->GetUseAnimation() == eUseAnim_Bow)
        {
            GlStateManager::Translatef(0.0f, -2.0f, -5.0f);
            GlStateManager::Rotatef(20.0f, 0.0f, 1.0f, 0.0f);
            GlStateManager::Scalef(s_heldItemScale, s_heldItemScale, -s_heldItemScale);
            GlStateManager::Rotatef(-100.0f, 1.0f, 0.0f, 0.0f);
            GlStateManager::Rotatef(45.0f, 0.0f, 1.0f, 0.0f);

            ItemInHandRenderer::RenderItem(heldItem->GetItem(), ((Player *) player)->GetHeldItemIcon(), lightColor);
        }
        else if (heldItem->GetItem()->IsHandEquipped())
        {
            if (blocking)
            {
                GlStateManager::Translatef(0.8f, 0.0f, 1.6f);
                GlStateManager::Rotatef(50.0f, 0.0f, 1.0f, 0.0f);
                GlStateManager::Rotatef(-10.0f, 1.0f, 0.0f, 0.0f);
                GlStateManager::Rotatef(60.0f, 0.0f, 0.0f, 1.0f);
            }

            GlStateManager::Translatef(0.0f, s_heldItemDropY, 0.0f);
            GlStateManager::Scalef(s_heldItemScale, s_heldItemScale, -s_heldItemScale);
            GlStateManager::Rotatef(-100.0f, 1.0f, 0.0f, 0.0f);
            GlStateManager::Rotatef(45.0f, 0.0f, 1.0f, 0.0f);

            ItemInHandRenderer::RenderItem(heldItem->GetItem(), ((Player *) player)->GetHeldItemIcon(), lightColor);
        }
        else
        {
            float scale = 6.0f;
            GlStateManager::Translatef(4.0f, -3.0f, 3.0f);
            GlStateManager::Scalef(scale, scale, scale);
            GlStateManager::Rotatef(-60.0f, 0.0f, 0.0f, 1.0f);
            GlStateManager::Rotatef(-90.0f, 1.0f, 0.0f, 0.0f);
            GlStateManager::Rotatef(-20.0f, 0.0f, 0.0f, 1.0f);
            GlStateManager::Rotatef(180.0f, 1.0f, 0.0f, 0.0f);

            ItemInHandRenderer::RenderItem(heldItem->GetItem(), ((Player *) player)->GetHeldItemIcon(), lightColor);
        }

        GlStateManager::SetGlint(0.0f);
        GlStateManager::PopMatrix();
    }

    GlStateManager::DisableBlend();
    GlStateManager::SetAlphaCutoff(alphaCutoff);

    GlStateManager::PopMatrix();

    if (m_inGui)
    {
        return;
    }

    if (!player->GetSharedFlag(eEntityFlag_Spectator))
    {
        RenderFire(entity, partialTicks);
    }

    if (m_renderBoundingBox)
    {
        RenderBoundingBox(entity, partialTicks);
    }
}

void PlayerRenderer::SetInGui(bool value)
{
    m_inGui = value;
}

#endif
