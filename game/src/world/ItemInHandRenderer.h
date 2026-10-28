#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <tuple>
#include <vector>

#include "../rendering/BufferBuilder.h"
#include "models/PlayerSkinModel.h"

#include <glm/mat4x4.hpp>

class Icon;
class Item;
class Texture;
class Model;
class ModelPart;
class LocalPlayer;

class ItemInHandRenderer
{
public:
    ItemInHandRenderer();
    ~ItemInHandRenderer();

    void Tick(LocalPlayer *player);
    void ResetEquip();
    void Render(const glm::dmat4 &projection, LocalPlayer *player, float partialTicks, uint32_t lightColor);

    static void RenderItem(Item *item, const Icon &icon, uint32_t lightColor);
    static void RenderDroppedItem(Item *item, uint32_t lightColor, bool flat);

private:
    static const int s_spriteResolution = 16;
    static const float s_spriteDepth;
    static const double s_thinBlockHeight;
    static const float s_equipSpeed;

    void EnsureArmModel(LocalPlayer *player);
    void RenderArm(LocalPlayer *player, float partialTicks, float equipHeight, uint32_t lightColor);
    void RenderHeldItem(LocalPlayer *player, float partialTicks, float equipHeight, uint32_t lightColor);
    void RenderFireOverlay();
    static void RenderItemSprite(const Icon &icon, uint32_t lightColor);

    static const std::vector<BufferBuilder::Vtx> &GetSpriteMesh(const Icon &icon);
    static void EmitSpriteQuad(std::vector<BufferBuilder::Vtx> *vertices, const Icon &icon, float brightness, const float corners[4][5]);

    static std::map<std::tuple<const Texture *, float, float>, std::vector<BufferBuilder::Vtx>> s_spriteMeshes;

    Model *m_armModel;
    ModelPart *m_arm;
    std::string m_armTexturePath;
    ePLAYER_SKIN_MODEL m_armSkinModel;

    float m_equipHeight;
    float m_equipHeightOld;
    int m_lastSlot;
    Item *m_selectedItem;
    int m_selectedAuxValue;
};
