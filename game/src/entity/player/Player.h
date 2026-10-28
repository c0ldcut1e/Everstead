#pragma once

#include <cstdint>
#include <string>

#include "../../world/DimensionId.h"
#include "../../world/block/BlockPos.h"
#include "../../world/inventory/AbstractContainer.h"
#include "../../world/inventory/AnvilContainer.h"
#include "../../world/inventory/CraftingContainer.h"
#include "../../world/models/PlayerSkinModel.h"
#include "../Inventory.h"
#include "../LivingEntity.h"
#include "FoodData.h"
#include "GameType.h"

class Player : public LivingEntity
{
public:
    explicit Player(Level *level, const std::wstring &name);
    ~Player() override;

    eENTITY_TYPE GetType() override;

    Inventory *GetInventory();
    AbstractContainer *GetOpenContainer() const;
    CraftingContainer *GetCraftingContainer() const;
    CraftingContainer *GetInventoryCraftingContainer() const;
    AnvilContainer *GetAnvilContainer() const;

    void Tick() override;
    int GetUpdateInterval() const override;

    void SetMoveInputs(float forward, float strafe);
    float GetMoveForwardInput() const;
    float GetMoveStrafeInput() const;
    void SetFlyVerticalInput(float value);

    void SetPlayerIndex(uint8_t playerIndex);
    uint8_t GetPlayerIndex() const;

    void SetName(const std::wstring &name) override;

    std::string GetScoreboardName() const override;

    void SetSkin(const std::string &texturePath, ePLAYER_SKIN_MODEL skinModel);
    const std::string &GetSkinTexturePath() const;
    ePLAYER_SKIN_MODEL GetSkinModel() const;

    void SetCape(const std::string &texturePath);
    const std::string &GetCapeTexturePath() const;
    glm::dvec3 GetCapePosition(float partialTicks) const;

    virtual void OpenContainer(AbstractContainer *container);
    virtual void CloseContainer();
    void StartUsingItem(const ItemInstance &item, int duration);
    void StopUsingItem();
    bool IsUsingItem() const;
    bool IsBlocking() const;
    bool IsDrawingBow() const;
    int GetUseItemDuration() const;
    int GetTicksUsingItem() const;
    const Icon &GetHeldItemIcon();

    void SetFlying(bool value);
    bool IsFlying() const;
    void SetGameType(eGAME_TYPE gameType);
    eGAME_TYPE GetGameType() const;
    void SetRespawnPosition(const BlockPos &pos, eDIMENSION_ID dimension, bool forced);
    void ClearRespawnPosition();
    bool HasRespawnPosition() const;
    bool IsRespawnForced() const;
    const BlockPos &GetRespawnPosition() const;
    eDIMENSION_ID GetRespawnDimension() const;
    void StartSleeping(const BlockPos &bedPos);
    void StopSleeping();
    bool IsSleeping() const;
    int GetSleepTimer() const;
    const BlockPos &GetBedPos() const;
    double GetBlockReach() const;
    float GetDestroyProgress(Block *block);
    bool CanHarvest(Block *block);
    double GetEntityReach() const;
    float GetTilt() const;
    float GetTiltOld() const;
    float GetBob() const;
    float GetBobOld() const;
    float GetFallTilt() const;
    float GetFallTiltOld() const;
    float GetWalkBob() const;
    float GetWalkBobOld() const;
    float GetFovModifier(float partialTicks) const;
    double GetStepHeight() const override;
    double GetEyeHeight() const override;
    FoodData *GetFoodData();
    int GetArmorValue() const override;
    int GetExperienceLevel() const;
    float GetExperienceProgress() const;
    int GetTotalExperience() const;
    int GetXpNeededForNextLevel() const;
    void SetExperience(float progress, int level, int total);
    void AddExperience(int amount);

    static const eENTITY_TYPE s_type = eEntityType_Player;

protected:
    void UpdateCapePosition();
    void SpawnSprintParticle(const glm::dvec3 &motion);
    void UpdateWaterSplash(const glm::dvec3 &motion);
    void TickUsingItem();
    void TickSleeping();
    virtual void CompleteUsingItem();
    void SpawnEatParticles(const ItemInstance &item, int count);

    bool m_wasInWater;
    bool m_waterChecked;
    float m_moveForwardInput;
    float m_moveStrafeInput;
    float m_flyVerticalInput;
    float m_tilt;
    float m_tiltOld;
    float m_bob;
    float m_bobOld;
    float m_fallTilt;
    float m_fallTiltOld;
    float m_walkBob;
    float m_walkBobOld;
    float m_fovModifier;
    float m_fovModifierOld;
    float m_ySlideOffset;
    float m_ySlideOffsetOld;
    uint8_t m_playerIndex;
    std::string m_skinTexturePath;
    ePLAYER_SKIN_MODEL m_skinModel;
    std::string m_capeTexturePath;
    glm::dvec3 m_capePosition;
    glm::dvec3 m_capeOldPosition;
    Inventory m_inventory;
    AbstractContainer *m_openContainer;
    CraftingContainer *m_craftingContainer;
    CraftingContainer *m_inventoryCraftingContainer;
    AnvilContainer *m_anvilContainer;
    eGAME_TYPE m_gameType;
    BlockPos m_respawnPosition;
    eDIMENSION_ID m_respawnDimension;
    bool m_hasRespawnPosition;
    bool m_respawnForced;
    bool m_sleeping;
    int m_sleepTimer;
    BlockPos m_bedPos;
    FoodData m_foodData;
    ItemInstance m_useItem;
    int m_useItemDuration;
    int m_experienceLevel;
    float m_experienceProgress;
    int m_experienceTotal;
};
