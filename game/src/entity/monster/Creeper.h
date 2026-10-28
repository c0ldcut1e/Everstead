#pragma once

#include "Monster.h"

class Creeper : public Monster
{
public:
    explicit Creeper(Level *level);

    eENTITY_TYPE GetType() override;
    bool DoHurtTarget(LivingEntity *target) override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;
    int GetSwellDir() const;
    void SetSwellDir(int swellDir);
    float GetSwelling(float partialTick) const;

    static const eENTITY_TYPE s_type = eEntityType_Creeper;

protected:
    void ServerTick() override;
    void ClientTick() override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;

private:
    void TickSwell();

    int m_oldSwell;
    int m_swell;
    int m_swellDir;
};
