#pragma once

#include <vector>

#include "../../world/block/BlockPos.h"
#include "../LivingEntity.h"

class FractureTrait;
class ServerPlayer;

class Arrow : public Entity
{
public:
    explicit Arrow(Level *level);

    eENTITY_TYPE GetType() override;
    void Tick() override;
    int GetUpdateInterval() const override;
    bool SendsVelocity() const override;
    bool IsPickable() const override;
    double GetRenderDistanceScale() const override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;

    void Shoot(LivingEntity *owner, float power, bool critical, bool creativeOnlyPickup);
    void ShootAt(LivingEntity *owner, LivingEntity *target, float velocity, float inaccuracy);
    void Launch(const glm::dvec3 &position, const glm::dvec3 &velocity);
    void Redirect(Entity *owner, const glm::dvec3 &velocity);
    void SetFractures(const std::vector<FractureTrait *> &fractures);
    void HomeOn(LivingEntity *target, const glm::dvec3 &velocity);
    int GetHomingTargetId() const;
    void ReturnToOwner();
    float GetShake(float partialTicks) const;
    void StartCollect(uint8_t playerIndex);
    bool IsCollecting() const;
    int GetOwnerId() const;
    uint8_t GetCollectorIndex() const;
    float GetCollectProgress(float partialTicks) const;

    static const eENTITY_TYPE s_type = eEntityType_Arrow;

private:
    void TickInGround();
    void TryPickup();
    void TickReturn();
    void TickHoming();
    bool GiveTo(ServerPlayer *player);
    void FaceVelocity();

    int m_ownerId;
    bool m_critical;
    bool m_creativeOnlyPickup;
    bool m_inGround;
    BlockPos m_stuckPos;
    uint32_t m_stuckBlock;
    int m_life;
    int m_flightTime;
    int m_shake;
    bool m_collecting;
    uint8_t m_collectorIndex;
    int m_collectAge;
    std::vector<FractureTrait *> m_fractures;
    int m_homingId;
    bool m_returning;
};
