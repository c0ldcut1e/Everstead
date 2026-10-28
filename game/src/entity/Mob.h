#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../utils/Random.h"
#include "LivingEntity.h"
#include "ai/control/BodyControl.h"
#include "ai/control/JumpControl.h"
#include "ai/control/LookControl.h"
#include "ai/control/MoveControl.h"
#include "ai/goal/GoalSelector.h"
#include "ai/pathfinder/PathNavigation.h"

class Item;
class Player;
struct LootProperty;

struct MobData
{
    int health;
    uint8_t flags;
    bool baby;
    bool inLove;
    uint8_t variant;
    int leashHolderId;
};

class Mob : public LivingEntity
{
public:
    explicit Mob(Level *level);
    ~Mob() override;

    eENTITY_TYPE GetType() override;

    void Tick() override;
    bool IsPickable() const override;
    double GetEyeHeight() const override;
    bool Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker) override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    virtual bool Interact(Player *player);
    bool InteractLeash(Player *player);
    virtual bool CanBeLeashed() const;
    int GetLeashHolderId() const;
    Entity *GetLeashHolder() const;
    void SetLeashHolder(Entity *holder);
    void DropLeash(bool dropItem);
    virtual bool IsBaby() const;
    virtual float GetWalkTargetValue(const BlockPos &pos) const;
    virtual void GetMobData(MobData *data) const;
    virtual void ApplyMobData(const MobData &data);
    virtual bool DoHurtTarget(LivingEntity *target);
    virtual void FinalizeSpawn();
    bool CanAddPassenger(Entity *passenger) const override;
    void PositionRider(Entity *passenger) override;

    GoalSelector *GetGoalSelector();
    LivingEntity *GetTarget() const;
    void SetTarget(LivingEntity *target);
    bool CanAttack(LivingEntity *target) const;
    bool CanSee(LivingEntity *entity) const;
    float GetBrightness() const;
    float GetWidth() const;
    int GetAttackTime() const;
    void SetAttackTime(int attackTime);
    MoveControl *GetMoveControl();
    JumpControl *GetJumpControl();
    LookControl *GetLookControl();
    PathNavigation *GetNavigation();
    Random &GetRandom() const;
    int GetNoActionTime() const;
    LivingEntity *GetLastHurtByMob() const;
    int64_t GetLastHurtByMobTimestamp() const;
    bool GetRandomPos(int horizontal, int vertical, glm::dvec3 *pos);
    void SetForwardInput(double forward);
    void SetSpeed(double speed);
    float GetBodyYaw() const;
    void SetBodyYaw(float yaw);
    void SetHeadYaw(float yaw);
    virtual void SetSize(float width, float height);
    void PlayAmbientSound();

    static constexpr float kMaxHeadPitch = 40.0f;
    static const eENTITY_TYPE s_type     = eEntityType_Mob;

protected:
    double GetMovementSpeed() const override;
    void Die(eDAMAGE_SOURCE source, LivingEntity *attacker) override;
    virtual void ServerTick();
    virtual void ServerAiStep();
    virtual void ClientTick();
    virtual std::string GetAmbientSound() const;
    virtual int GetAmbientSoundInterval() const;
    void DropDeathLoot();
    void TickLeash();
    void RestoreLeash();
    virtual void GetLootProperties(std::vector<LootProperty> *properties) const;
    virtual int GetExperienceReward();
    virtual bool RemoveWhenFarAway() const;
    virtual bool CanDropLoot() const;
    virtual double GetRideHeight() const;
    float GetVoicePitch() const override;
    void SpawnAtLocation(Item *item, int count);

    GoalSelector m_goalSelector;
    GoalSelector m_targetSelector;
    MoveControl m_moveControl;
    JumpControl m_jumpControl;
    LookControl m_lookControl;
    BodyControl m_bodyControl;
    PathNavigation m_navigation;
    float m_width;
    float m_height;
    double m_speed;
    double m_forwardInput;
    double m_strafeInput;
    int m_noActionTime;
    int m_ambientSoundTime;
    int m_lastHurtByMobId;
    int m_lastHurtByMobTime;
    int64_t m_lastHurtByMobTimestamp;
    int m_targetId;
    int m_attackTime;
    int m_lastHurtByPlayerTime;
    MobData m_sentData;
    int m_leashHolderId;
    std::unique_ptr<CompoundTag> m_leashTag;
    int m_leashRestoreTime;
};
