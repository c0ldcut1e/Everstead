#pragma once

#include <cstdint>
#include <vector>

#include "../../network/packet/c2s/PlayerActionC2SPacket.h"
#include "../../utils/AABB.h"
#include "../../utils/HitResult.h"
#include "../../utils/Random.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockPos.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/inventory/Container.h"
#include "../../world/inventory/FurnaceContainer.h"
#include "../../world/inventory/ResonantSocketContainer.h"
#include "../../world/inventory/ResonatorContainer.h"
#include "../../world/particle/ParticleSpawnParams.h"
#include "Player.h"
#include "RemotePlayer.h"

enum eTHIRD_PERSON_VIEW : uint8_t
{
    eThirdPersonView_FirstPerson = 0,
    eThirdPersonView_Back        = 1,
    eThirdPersonView_Front       = 2,
};

enum eRECIPE_VIEW_TYPE : uint8_t
{
    eRecipeViewType_Crafting  = 0,
    eRecipeViewType_Furnace   = 1,
    eRecipeViewType_Break     = 2,
    eRecipeViewType_Resonator = 3,
};

class Recipe;

struct RecipeViewPage
{
    eRECIPE_VIEW_TYPE type;
    const Recipe *recipe;
    Item *input;
    ItemInstance source;
    ItemInstance result;
    bool kill;
};

class LocalPlayer : public Player
{
public:
    LocalPlayer(Level *level, const std::wstring &name);
    ~LocalPlayer() override;

    eENTITY_TYPE GetType() override;

    void Update(float partialTicks);
    glm::dvec3 GetEyePosition(float partialTicks) const;
    RemotePlayer *GetRenderPlayer() const;
    void Tick() override;
    void HurtTo(int health);
    void AnimateServerHurt();
    void SetClickInputs(bool attackPressed, bool attackHeld, bool destroyHeld, bool usePressed, bool useHeld, bool useItemHeld);

    void ClearInputs();
    void SetJumpHeld(bool value);
    void RegisterJumpPress();
    void SetSprintHeld(bool value);
    void SetSneakHeld(bool value);
    void SetForwardTaps(int count);
    void ApplyLookInput(double dx, double dy);
    void ResetRotation(float yaw, float pitch);
    HitResult *Pick() const;
    void BreakTargetedBlock(bool canAttack);
    const BlockPos &GetDestroyPos() const;
    float GetCaveMood() const;
    int GetCaveSoundsPlayed() const;
    int GetDestroyStage() const;
    void PlaceTargetedBlock();
    void PickTargetedBlock(bool copyData);
    void DropSelectedItem(bool all);
    void ScrollFlySpeed(int steps);
    void OpenContainer(AbstractContainer *container) override;
    void CloseContainer() override;
    void ViewRecipe(const Item *item);
    void FillRecipeView();
    void SetRecipeViewPage(int page);
    CraftingContainer *GetRecipeView() const;
    eRECIPE_VIEW_TYPE GetRecipeViewType() const;
    bool IsKillRecipeView() const;
    int GetRecipeViewPage() const;
    int GetRecipeViewPageCount() const;
    FurnaceContainer *GetFurnaceContainer();
    ResonantSocketContainer *GetResonantSocketContainer();
    ResonatorContainer *GetResonatorContainer();
    void OpenChest(uint8_t menuId, int rows);
    uint8_t GetChestMenuId() const;
    void Swing() override;
    void SpawnDestroyParticlesAt(const BlockPos &pos);
    void SpawnCrackParticle(const BlockPos &pos, Direction *face);
    void CycleThirdPersonView();
    eTHIRD_PERSON_VIEW GetThirdPersonView() const;
    bool IsFirstPerson() const;
    bool IsEyeInWater() const;
    void PlaySound(const ResourceLocation &location, float volume, float pitch);

    static const eENTITY_TYPE s_type = eEntityType_LocalPlayer;

protected:
    virtual void UseBlock(const BlockPos &pos, Direction *face);
    virtual void UseItemOn(const BlockPos &pos, Direction *face);
    virtual void SetBlockByPlayer(const BlockPos &pos, Block *block, Direction *placedAgainst, uint16_t data);

private:
    bool IsOpenBlockSpace(const BlockPos &pos) const;
    bool IsHeadspaceFree(const BlockPos &pos, int height) const;
    void PushOutOfBlock(double x, double y, double z);
    void PushOutOfBlocks();
    void WearHeldItem(const BlockPos &pos, Block *block);
    void DestroyBlock(const BlockPos &pos);
    Direction *GetParticleDirection(Block *block) const;
    uint32_t MultiplyColorRgb(uint32_t color, uint32_t tint) const;
    ParticleSpawnParams GetTerrainParticleParams(const BlockPos &pos, Block *block, const Chunk *chunk) const;
    void SpawnDestroyParticles(const BlockPos &pos, const AABB &aabb, Block *block, const Chunk *chunk);
    bool PlaceBlock(const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, Block *block, BlockPos *outPlacedPos);
    bool PlaceHeldBlock(HitResult *hit, ItemInstance *selectedItem, Block *block);
    void UpdateSprinting();
    void UpdateFlyToggle();
    void UpdateCaveMood();
    void CompleteUsingItem() override;
    void SendUseItemAction(ePLAYER_ACTION action);
    void UpdateClickRepeat();
    void StartDestroy(const BlockPos &pos);
    void ContinueDestroy();
    void StopDestroy(bool abort);

    static const int s_caveSoundCount          = 15;
    static constexpr float s_caveMoodTickDelay = 6000.0f;
    static const float s_sprintForwardThreshold;
    static const float s_sprintFovModifier;
    static const float s_flyFovModifier;
    static const int s_sprintTriggerWindowTicks = 7;
    static const int s_flyToggleWindowTicks     = 7;
    static constexpr double s_flySpeedStep      = 0.005;
    static constexpr double s_maxFlySpeed       = 0.2;
    static const int s_clickRepeatTicks         = 5;

    RemotePlayer *m_renderPlayer;
    eTHIRD_PERSON_VIEW m_thirdPersonView;

    bool m_sprintKeyDown;
    int m_forwardTapsThisTick;
    int m_sprintTriggerTime;
    int m_jumpPressCount;
    int m_jumpTriggerTime;
    bool m_attackHeld;
    bool m_useHeld;
    int m_attackRepeatTime;
    int m_useRepeatTime;
    bool m_destroyHeld;
    bool m_isDestroying;
    BlockPos m_destroyPos;
    float m_destroyProgress;
    int m_destroyTicks;
    int m_destroyDelay;
    int m_destroyStage;
    float m_caveMood;
    bool m_inTear;
    int m_caveSoundsPlayed;
    Random m_caveRandom;
    CraftingContainer *m_recipeView;
    AbstractContainer *m_recipeViewParent;
    std::vector<RecipeViewPage> m_recipeViewPages;
    int m_recipeViewPage;
    uint32_t m_recipeViewStartMs;
    Container *m_chestContainer;
    uint8_t m_chestMenuId;
    FurnaceContainer m_furnaceContainer;
    ResonantSocketContainer m_resonantSocketContainer;
    ResonatorContainer m_resonatorContainer;
};
