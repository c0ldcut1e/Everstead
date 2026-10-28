#pragma once

#include <cstdint>

#include "../../utils/Random.h"
#include "../Entity.h"

class Player;

class Boat : public Entity
{
public:
    explicit Boat(Level *level);
    ~Boat() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void HandleEntityEvent(uint8_t event) override;

    bool IsSolid() const override;
    bool CanAddPassenger(Entity *passenger) const override;
    void PositionRider(Entity *passenger) override;
    bool IsControlledByRider() const override;

    bool Hurt(int amount, Player *attacker);
    void SetWoodType(int woodType);
    int GetWoodType() const;
    void SetPaddleState(bool left, bool right);
    bool GetPaddleState(int side) const;
    float GetRowingTime(int side, float partialTicks) const;
    int GetTimeSinceHit() const;
    int GetHurtDir() const;
    float GetDamage() const;

    static constexpr int kWoodTypeCount        = 7;
    static constexpr const char *kWoodTypes[7] = {"oak", "spruce", "birch", "acacia", "jungle", "cherry", "dark_oak"};
    static constexpr uint8_t kHurtEvent        = 11;
    static const eENTITY_TYPE s_type           = eEntityType_Boat;

private:
    enum eSTATUS
    {
        eStatus_InWater,
        eStatus_UnderWater,
        eStatus_UnderFlowingWater,
        eStatus_OnLand,
        eStatus_InAir,
    };

    eSTATUS GetStatus();
    bool CheckInWater();
    bool GetUnderwaterStatus(bool *flowing) const;
    float GetGlide() const;
    double GetWaterLevelAbove() const;
    void UpdateMotion();
    void ControlBoat(Player *player);

    int m_woodType;
    int m_timeSinceHit;
    int m_hurtDir;
    float m_damage;
    bool m_paddleStates[2];
    float m_paddlePositions[2];
    float m_deltaRotation;
    float m_outOfControlTicks;
    double m_waterLevel;
    double m_lastYd;
    float m_glide;
    eSTATUS m_status;
    eSTATUS m_previousStatus;
};
