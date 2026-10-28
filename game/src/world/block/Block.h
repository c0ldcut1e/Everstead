#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../core/ResourceLocation.h"
#include "../../rendering/Texture.h"
#include "../../utils/AABB.h"
#include "../../utils/Direction.h"
#include "../../utils/Random.h"
#include "BlockPos.h"

#include <glm/vec2.hpp>

class Chunk;
class Entity;
class Item;
class ItemInstance;
class Level;

class Block
{
public:
    enum eRENDER_SHAPE : uint8_t
    {
        eRenderShape_CUBE  = 0,
        eRenderShape_CROSS = 1,
        eRenderShape_TORCH = 2,
        eRenderShape_BOXES = 3,
        eRenderShape_FIRE  = 4,
    };

    enum eCROSS_JITTER : uint8_t
    {
        eCrossJitter_None       = 0,
        eCrossJitter_Horizontal = 1,
        eCrossJitter_Full       = 2,
    };

    enum eTOOL_TYPE : uint8_t
    {
        eToolType_None    = 0,
        eToolType_Pickaxe = 1,
        eToolType_Shovel  = 2,
        eToolType_Axe     = 3,
    };

    struct Property
    {
        std::string name;
        uint8_t valueCount;
        uint8_t shift;
        uint16_t mask;
    };

    struct UVRect
    {
        float u0;
        float v0;
        float u1;
        float v1;
    };

    struct RenderPlane
    {
        glm::dvec3 start;
        glm::dvec3 end;
        glm::dvec3 halfWidth;
        float u0;
        float v0;
        float u1;
        float v1;
    };

    Block();
    Block(bool solid, const std::string &texturePath);
    virtual ~Block() = default;

    static Block *ById(uint32_t id);
    static Block *ByName(const ResourceLocation &location);

    virtual void OnPlace(Level *level, const BlockPos &pos);
    virtual void OnBreak(Level *level, const BlockPos &pos);
    virtual void OnRemoved(Level *level, const BlockPos &pos, uint16_t data);
    virtual void OnNeighborChanged(Level *level, const BlockPos &pos);
    virtual void OnClientPlace(Level *level, const BlockPos &pos);
    virtual void UpdateShape(Level *level, const BlockPos &pos);
    virtual void OnNeighborStateChanged(Level *level, const BlockPos &pos, Direction *direction);
    void DropAsItem(Level *level, const BlockPos &pos, uint16_t data, const ItemInstance *tool = nullptr) const;
    virtual void Tick(Level *level, const BlockPos &pos);
    virtual void TriggerEvent(Level *level, const BlockPos &pos, int id);
    virtual float GetLiquidHeight(uint8_t data) const;
    virtual glm::dvec3 GetFlow(Level *level, const BlockPos &pos) const;
    virtual void GetBoxes(uint16_t data, std::vector<AABB> *out) const;
    virtual void GetPlacedBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const;
    virtual void GetRenderBoxes(uint16_t data, std::vector<AABB> *out) const;
    virtual void GetItemBoxes(std::vector<AABB> *out) const;
    virtual bool HasBoxFace(uint16_t data, size_t boxIndex, Direction *direction) const;
    virtual bool IsBoxFaceDoubleSided(uint16_t data, size_t boxIndex, Direction *direction) const;
    virtual bool HasAmbientOcclusion() const;
    virtual bool HasDirectionalShade() const;
    virtual void GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const;
    virtual void GetPlacedCollisionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const;
    virtual void RandomTick(Level *level, const BlockPos &pos);
    virtual void AnimateTick(Level *level, const BlockPos &pos, Random *random);
    virtual bool CanBeReplaced(uint16_t data) const;
    virtual uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const;
    virtual Block *GetCombinedBlock(uint16_t existingData, uint16_t placedData) const;
    virtual uint16_t GetItemData() const;
    virtual bool IsUsable(uint16_t data) const;
    virtual bool OnUse(Level *level, const BlockPos &pos);
    virtual void OnAttack(Level *level, const BlockPos &pos);
    virtual UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const;
    virtual void AdjustBoxFaceUV(uint16_t data, const AABB &box, Direction *direction, float *u0, float *v0, float *u1, float *v1) const;
    virtual void GetRenderPlanes(uint16_t data, std::vector<RenderPlane> *out) const;

    void SetTexture(Direction *direction, Texture *texture);
    Texture *GetTexture(Direction *direction) const;
    void SetTexturePath(Direction *direction, const std::string &path);
    void SetFaceTexture(Direction *direction, const std::string &path);

    int AddProperty(const std::string &name, int valueCount);
    int FindProperty(const std::string &name) const;
    const std::vector<Property> &GetProperties() const;
    int GetPropertyValue(uint16_t data, int property) const;
    uint16_t WithPropertyValue(uint16_t data, int property, int value) const;

    void SetHorizontalFacing();
    virtual uint16_t GetPlacementData(Direction *placedAgainst) const;
    virtual Direction *GetAttachment(uint16_t data) const;

    void SetPillar(const std::string &topPath);
    bool IsPillar() const;
    std::string GetRotatedSidePath() const;
    void SetRotatedSideAtlasUVRect(float u0, float v0, float u1, float v1);

    void SetSideOverlay(const std::string &path);
    bool HasSideOverlay() const;
    const std::string &GetSideOverlayPath() const;
    void SetSideOverlayAtlasUVRect(float u0, float v0, float u1, float v1);
    UVRect GetSideOverlayAtlasUVRect() const;

    void SetSecondaryTexture(const std::string &path);
    bool HasSecondaryTexture() const;
    const std::string &GetSecondaryTexturePath() const;
    void SetSecondaryAtlasUVRect(float u0, float v0, float u1, float v1);
    UVRect GetSecondaryAtlasUVRect() const;
    int AddTextureVariant(const std::string &path);
    const std::vector<std::string> &GetTextureVariantPaths() const;
    void SetTextureVariantAtlasUVRect(int index, float u0, float v0, float u1, float v1);
    UVRect GetTextureVariantAtlasUVRect(int index) const;
    virtual UVRect GetCrossAtlasUVRect(uint16_t data) const;
    virtual int GetCrossTextureVariant(uint16_t data) const;
    void SetCrossJitter(eCROSS_JITTER crossJitter);
    eCROSS_JITTER GetCrossJitter() const;
    virtual glm::dvec3 GetCrossOffset(const BlockPos &pos, uint16_t data) const;
    void SetCrossFollowsAttachment(bool crossFollowsAttachment);
    bool CrossFollowsAttachment() const;
    const std::string &GetTexturePath(Direction *direction) const;

    void SetBiomeTint();
    void SetBiomeTint(Direction *direction);
    void SetFoliageTint();
    void SetFixedTint(uint32_t color);
    uint32_t GetItemTint() const;
    virtual uint32_t GetDataTint(uint16_t data) const;
    static uint32_t GetItemGrassTint();
    bool HasBiomeTint(Direction *direction) const;

    uint32_t ResolveTint(Direction *direction, Level *level, const Chunk *chunk, int localX, int localZ) const;
    uint32_t ResolveTint(Direction *direction, uint32_t grassColor, uint32_t foliageColor) const;

    const ResourceLocation &GetResourceLocation() const;

    void SetBreakSound(const std::string &breakSound);
    void SetBreakSound(const std::string &breakSound, int variants);
    const std::string &GetBreakSound() const;
    int GetBreakSoundVariants() const;
    std::string GetStepSound(int *outVariants) const;
    void SetPlaceSound(const std::string &placeSound, int variants, float pitch);
    const std::string &GetPlaceSound() const;
    int GetPlaceSoundVariants() const;
    float GetPlaceSoundPitch() const;
    void SetNoteInstrument(const std::string &noteInstrument);
    const std::string &GetNoteInstrument() const;

    uint32_t GetId() const
    {
        return m_id;
    }

    void SetId(uint32_t id);

    bool IsSolid() const;
    bool IsSignalConductor() const;

    // Direction points from this block toward the block receiving the signal
    virtual int GetSignal(uint16_t data, Direction *direction) const;
    virtual int GetDirectSignal(uint16_t data, Direction *direction) const;
    virtual bool IsSignalSource() const;
    // Direction points from the wire toward this block
    virtual bool CanConnectRedstone(uint16_t data, Direction *direction) const;
    virtual bool HasAnalogOutput() const;
    virtual int GetAnalogOutput(Level *level, const BlockPos &pos) const;
    bool BlocksLight() const;
    void SetLightOpacity(int opacity);
    int GetLightOpacity() const;
    void SetCollidable(bool collidable);
    bool IsCollidable() const;
    void SetSelectable(bool selectable);
    bool IsSelectable() const;

    bool IsTransparent() const;
    void SetTransparent(bool transparent);
    bool CullsSameNeighbor() const;
    void SetCullsSameNeighbor(bool cullsSameNeighbor);

    bool IsTranslucent() const;
    void SetTranslucent(bool translucent);
    bool IsLiquid() const;
    virtual bool IsLava() const;
    void SetLiquid(bool liquid);

    bool IsReplaceable() const;
    void SetRandomTicking(bool randomTicking);
    void SetLiquidReplaceable(bool liquidReplaceable);
    bool IsLiquidReplaceable() const;
    void SetBlocksLiquid(bool blocksLiquid);
    bool BlocksLiquid() const;
    bool IsRandomTicking() const;
    void SetSlipperiness(float slipperiness);
    float GetSlipperiness() const;
    virtual void OnEntityLanded(Entity *entity, double velocityY) const;
    virtual void OnEntityWalk(Entity *entity) const;
    virtual void OnProjectileHit(Level *level, const BlockPos &pos, Entity *projectile, const glm::dvec3 &hitPos, Direction *face);
    virtual float GetFallDamageMultiplier(Entity *entity) const;
    void SetReplaceable(bool replaceable);

    bool NeedsSupportBelow() const;
    void SetNeedsSupportBelow(bool needsSupport);
    void SetNeedsAttachedSupport(bool needsSupport);
    void SetNeedsCenterSupport(bool needsSupport);
    void SetSupportBlocks(const std::vector<std::string> &supportBlocks);
    virtual bool CanSurvive(Level *level, const BlockPos &pos, Direction *attachment) const;
    virtual BlockPos GetOtherPos(const BlockPos &pos, uint16_t data) const;
    bool IsFaceSturdy(uint16_t data, Direction *face) const;
    bool CanSupportCenter(uint16_t data, Direction *face) const;
    bool CanSupportRigid(uint16_t data, Direction *face) const;

    void SetWallVariant(Block *wallVariant);
    virtual Block *GetPlacementBlock(Direction *face);

    void SetAABB(const AABB &aabb);
    const AABB &GetAABB() const;
    void SetInteractionAABB(const AABB &aabb);
    const AABB &GetInteractionAABB() const;
    bool HasInteractionAABB() const;
    void SetInteractionAttachmentOffset(float offset);
    float GetInteractionAttachmentOffset() const;
    AABB GetPlacedAABB(const BlockPos &pos, Direction *attachmentFace) const;
    virtual AABB GetPlacedAABB(const BlockPos &pos, uint16_t data) const;
    virtual void GetSelectionBoxes(const Level *level, const BlockPos &pos, uint16_t data, std::vector<AABB> *out) const;
    bool RendersAsItemBlock() const;
    void SetFlatItem(bool flatItem);
    void SetClimbable(bool climbable);
    bool IsClimbable() const;

    void SetFlammable(int encouragement, int flammability);
    int GetFireEncouragement() const;
    int GetFlammability() const;
    void SetExplosionResistance(float resistance);
    float GetExplosionResistance() const;
    void SetDestroyTime(float destroyTime);
    float GetDestroyTime() const;
    void SetTool(eTOOL_TYPE toolType, bool requiresTool, int harvestLevel);
    eTOOL_TYPE GetToolType() const;
    bool RequiresTool() const;
    int GetHarvestLevel() const;
    virtual void OnExploded(Level *level, const BlockPos &pos);
    virtual bool DropsFromExplosion() const;

    void SetLightEmission(uint8_t value);
    uint8_t GetLightEmission() const;

    void SetLightColor(uint8_t r, uint8_t g, uint8_t b);
    void GetLightColor(uint8_t *r, uint8_t *g, uint8_t *b) const;

    void SetRenderShape(eRENDER_SHAPE type);
    eRENDER_SHAPE GetRenderShape() const;
    void SetEntityRendered(bool entityRendered);
    bool IsEntityRendered() const;
    virtual const char *GetBlockEntityId() const;

    void SetUVRect(Direction *direction, float u0, float v0, float u1, float v1);
    UVRect GetUVRect(Direction *direction) const;
    void SetAtlasUVRect(Direction *direction, float u0, float v0, float u1, float v1);
    UVRect GetAtlasUVRect(Direction *direction) const;
    virtual UVRect GetAtlasUVRect(Direction *direction, uint16_t data) const;

    void SetWallMountedTransform(float tiltDegrees, float wallInset);
    bool HasWallMountedTransform() const;
    float GetWallMountedTiltDegrees() const;
    float GetWallMountedInset() const;
    void SetTorchPlanes();
    bool HasTorchPlanes() const;

protected:
    void SpawnDrop(Level *level, const BlockPos &pos, Item *item, int auxValue) const;
    bool CoversFace(uint16_t data, Direction *face, double areaMin, double areaMax) const;
    void GetBoxFaceCorners(const AABB &box, Direction *direction, glm::dvec3 *corners) const;
    glm::vec2 GetModelFaceAxes(Direction *face, const glm::vec3 &model) const;
    // The mesher only takes an axis aligned UV rect, so faces whose texture runs across it use the rotated copy that follows every texture
    void ResolveFaceUV(int texture, glm::vec2 *st, int *outTexture, float *u0, float *v0, float *u1, float *v1) const;

    static thread_local Random s_dropRandom;

    uint32_t m_id;
    std::unordered_map<Direction *, Texture *> m_textures;
    std::unordered_map<Direction *, std::string> m_texturePaths;
    std::unordered_map<Direction *, bool> m_biomeTintFaces;
    bool m_foliageTint;
    int m_lightOpacity;
    bool m_hasFixedTint;
    uint32_t m_fixedTint;
    std::string m_breakSound;
    int m_breakSoundVariants;
    std::vector<std::string> m_supportBlocks;
    std::string m_placeSound;
    int m_placeSoundVariants;
    float m_placeSoundPitch;
    std::string m_noteInstrument;
    bool m_solid;
    bool m_collidable;
    bool m_selectable;
    bool m_transparent;
    bool m_cullsSameNeighbor;
    bool m_translucent;
    bool m_liquid;
    bool m_replaceable;
    bool m_randomTicking;
    float m_slipperiness;
    bool m_liquidReplaceable;
    bool m_blocksLiquid;
    bool m_needsSupportBelow;
    bool m_needsCenterSupport;
    bool m_needsAttachedSupport;
    bool m_flatItem;
    bool m_climbable;
    Block *m_wallVariant;
    AABB m_aabb;
    AABB m_interactionAABB;
    bool m_hasInteractionAABB;
    float m_interactionAttachmentOffset;
    int m_fireEncouragement;
    int m_flammability;
    float m_explosionResistance;
    float m_destroyTime;
    eTOOL_TYPE m_toolType;
    bool m_requiresTool;
    int m_harvestLevel;
    uint8_t m_lightEmission;
    uint8_t m_lightR;
    uint8_t m_lightG;
    uint8_t m_lightB;
    eRENDER_SHAPE m_renderShape;
    bool m_entityRendered;
    std::unordered_map<Direction *, UVRect> m_uvRects;
    std::unordered_map<Direction *, UVRect> m_atlasUVRects;
    bool m_hasWallMountedTransform;
    float m_wallMountedTiltDegrees;
    float m_wallMountedInset;
    bool m_hasTorchPlanes;
    std::vector<Property> m_properties;
    int m_facingProperty;
    int m_axisProperty;
    UVRect m_rotatedSideAtlasUVRect;
    std::string m_sideOverlayPath;
    UVRect m_sideOverlayAtlasUVRect;
    std::string m_secondaryTexturePath;
    UVRect m_secondaryAtlasUVRect;
    std::vector<std::string> m_textureVariantPaths;
    std::vector<UVRect> m_textureVariantAtlasUVRects;
    eCROSS_JITTER m_crossJitter;
    bool m_crossFollowsAttachment;
};
