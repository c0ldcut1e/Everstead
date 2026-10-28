#pragma once

#include "../Mob.h"

class Squid : public Mob
{
public:
    explicit Squid(Level *level);

    eENTITY_TYPE GetType() override;
    bool CanBeLeashed() const override;

    float GetXBodyRot(float partialTicks) const;
    float GetZBodyRot(float partialTicks) const;
    float GetTentacleAngle(float partialTicks) const;

    static const eENTITY_TYPE s_type = eEntityType_Squid;

protected:
    void ServerTick() override;
    void ClientTick() override;
    void Travel() override;
    bool TickAirSupply() override;
    bool RemoveWhenFarAway() const override;
    int GetExperienceReward() override;
    int GetAmbientSoundInterval() const override;
    float GetSoundVolume() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;

private:
    void TickSwim(const glm::dvec3 &motion);

    float m_xBodyRot;
    float m_xBodyRotOld;
    float m_zBodyRot;
    float m_zBodyRotOld;
    float m_tentacleMovement;
    float m_tentacleAngle;
    float m_tentacleAngleOld;
    float m_speed;
    float m_tentacleSpeed;
    float m_rotateSpeed;
    glm::dvec3 m_swimTarget;
};
