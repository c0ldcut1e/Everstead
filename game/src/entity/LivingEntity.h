#pragma once

#include <cstdint>
#include <string>

#include "Entity.h"

enum eDAMAGE_SOURCE : uint8_t
{
    eDamageSource_Generic           = 0,
    eDamageSource_InFire            = 1,
    eDamageSource_OnFire            = 2,
    eDamageSource_Lava              = 3,
    eDamageSource_Fall              = 4,
    eDamageSource_OutOfWorld        = 5,
    eDamageSource_Player            = 6,
    eDamageSource_Stalagmite        = 7,
    eDamageSource_FallingStalactite = 8,
    eDamageSource_Explosion         = 9,
    eDamageSource_OutsideBorder     = 10,
    eDamageSource_Starve            = 11,
    eDamageSource_Drown             = 12,
    eDamageSource_Thrown            = 13,
    eDamageSource_Mob               = 14,
    eDamageSource_Arrow             = 15,
    eDamageSource_LightningBolt     = 16,
    eDamageSource_HotFloor          = 17,
    eDamageSource_PricklyVine       = 18,
    eDamageSource_Cactus            = 19,
    eDamageSource_Cramming          = 20,
    eDamageSource_SweetBerryBush    = 21,
    eDamageSource_Rift              = 22,
    eDamageSource_Anvil             = 23,
};

class LivingEntity : public Entity
{
public:
    explicit LivingEntity(Level *level);
    ~LivingEntity() override;

    eENTITY_TYPE GetType() override;

    virtual double GetEyeHeight() const = 0;
    virtual void Swing();
    float GetAttackAnim(float partialTicks) const;
    int GetHealth() const;
    int GetMaxHealth() const;
    void SetHealth(int health);
    int GetLastHealth() const;
    int GetInvulnerableTime() const;
    float GetFallDistance() const;
    void SetFallDistance(float fallDistance);
    void SetInvulnerableTime(int ticks);
    float GetHurtAnim(float partialTicks) const;
    int GetHurtTime() const;
    int GetDeathTime() const;
    void AnimateHurt();
    void AnimateCrit();
    int GetAirSupply() const;
    virtual bool TickAirSupply();
    virtual int GetArmorValue() const;
    virtual bool Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker);
    virtual void Heal(int amount);
    virtual void Knockback(const glm::dvec3 &source, const glm::dvec3 &push);
    void SetOnFire(int seconds);
    void ApplyNetworkState(const glm::dvec3 &position, float yaw, float pitch) override;
    void ApplyRenderState(const glm::dvec3 &position, float yaw, float pitch);
    void ApplyHeadYaw(float yaw);
    void SetInstantHeadRotation(float yaw, float pitch);
    float GetHeadYaw() const override;
    float GetRenderBodyYaw(float partialTicks) const;
    float GetRenderHeadYaw(float partialTicks) const;
    float GetLimbSwing() const;
    float GetLimbSwingOld() const;
    float GetLimbSwingAmount() const;
    float GetLimbSwingAmountOld() const;

    static const eENTITY_TYPE s_type  = eEntityType_LivingEntity;
    static const int s_totalAirSupply = 300;
    static bool IsArmorDamage(eDAMAGE_SOURCE source);
    static const int s_invulnerableDuration = 20;
    static const int s_hurtDuration         = 10;

protected:
    bool CanStartSwing() const;
    void UpdateSwing();
    void UpdateFall(const glm::dvec3 &position, double moveY, bool grounded);
    void UpdateStepSound(const glm::dvec3 &moved, bool grounded);
    void TickHurt();
    void TickCrit();
    void PushEntities();
    bool CanPush(Entity *entity);
    void TickCramming();
    void TickDeath();
    virtual void TickFire();
    virtual void CauseFallDamage(float distance);
    virtual int AbsorbDamage(eDAMAGE_SOURCE source, int damage);
    virtual void OnHurt(eDAMAGE_SOURCE source);
    virtual std::string GetHurtSound() const;
    virtual std::string GetDeathSound() const;
    virtual float GetSoundVolume() const;
    virtual float GetVoicePitch() const;
    virtual void Die(eDAMAGE_SOURCE source, LivingEntity *attacker);
    void TickRemote() override;
    void UpdateLimbSwing(float horizontalSpeed);
    void UpdateBodyYaw(float deltaX, float deltaZ);
    static uint32_t MultiplyTint(uint32_t color, uint32_t tint);

    static const int s_swingDuration;
    static const float s_maxHeadYaw;
    static const float s_moveThresholdSquared;

    bool m_swinging;
    int m_swingTime;
    float m_attackAnim;
    float m_attackAnimOld;
    float m_fallDistance;
    int m_health;
    int m_lastHealth;
    int m_lastHurt;
    int m_invulnerableTime;
    int m_hurtTime;
    int m_critTime;
    int m_deathTime;
    int m_airSupply;
    int m_fireTicks;
    int m_damageSpill;
    float m_limbSwing;
    float m_limbSwingOld;
    float m_limbSwingAmount;
    float m_limbSwingAmountOld;
    float m_bodyYaw;
    float m_bodyYawOld;
    float m_headYaw;
    float m_headYawOld;
    bool m_headYawChanged;
    float m_stepDistance;
    int m_nextStep;
};
