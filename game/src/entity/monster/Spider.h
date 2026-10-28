#pragma once

#include "Monster.h"

class Spider : public Monster
{
public:
    explicit Spider(Level *level);

    eENTITY_TYPE GetType() override;
    bool IsOnLadder() const override;
    void FinalizeSpawn() override;

    static const eENTITY_TYPE s_type = eEntityType_Spider;

protected:
    void ServerTick() override;
    double GetRideHeight() const override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
};
