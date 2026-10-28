#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "../../world/models/PlayerSkinModel.h"
#include "../EntityRenderer.h"

class Model;

class PlayerRenderer : public EntityRenderer
{
public:
    PlayerRenderer();

    void Render(const Entity *entity, float partialTicks, uint32_t lightColor) const override;

    void SetInGui(bool value);

    static const float s_heldItemArmOffsetX;
    static const float s_heldItemArmOffsetY;
    static const float s_heldItemArmOffsetZ;
    static const float s_heldItemScale;

private:
    static const float s_heldItemDropY;
    static const float s_heldBlockOffsetY;
    static const float s_heldBlockOffsetZ;
    static const float s_heldBlockScale;
    static const float s_heldArmPitch;
    static const uint32_t s_spectatorAlpha = 0x26000000;

    Model *GetSkinModel(const std::string &texturePath, ePLAYER_SKIN_MODEL skinModel) const;
    Model *GetCapeModel(const std::string &texturePath) const;

    bool m_inGui;
    mutable std::unordered_map<std::string, std::unique_ptr<Model>> m_skinModels;
    mutable std::unordered_map<std::string, std::unique_ptr<Model>> m_capeModels;
};
