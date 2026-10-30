#pragma once

#include <cstdint>
#include <deque>
#include <memory>

#include "../../world/block/BlockPos.h"
#include "../../world/inventory/CompoundContainer.h"
#include "../../world/inventory/Container.h"
#include "../Entity.h"

class Player;
class ServerPlayer;

class Minecart : public Entity
{
public:
    enum eMINECART_TYPE
    {
        eMinecartType_Rideable,
        eMinecartType_Chest,
        eMinecartType_Furnace,
        eMinecartType_Tnt,
        eMinecartType_Hopper,
    };

    struct NetworkStep
    {
        glm::dvec3 position;
        float yaw;
        float pitch;
    };

    explicit Minecart(Level *level);
    ~Minecart() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void HandleEntityEvent(uint8_t event) override;
    int GetUpdateInterval() const override;
    void ApplyNetworkState(const glm::dvec3 &position, float yaw, float pitch) override;

    bool CanAddPassenger(Entity *passenger) const override;
    void PositionRider(Entity *passenger) override;
    glm::dvec3 GetDismountPosition(Entity *passenger) const override;

    bool Hurt(int amount, Player *attacker);
    void PrimeFuse(int fuse);
    bool Interact(ServerPlayer *player);
    void SetMinecartType(int type);
    int GetMinecartType() const;
    bool IsLit() const;
    int GetFuse() const;
    Container *GetContainer() const;
    int GetTimeSinceHit() const;
    int GetHurtDir() const;
    float GetDamage() const;
    bool GetPos(glm::dvec3 position, glm::dvec3 *out) const;
    bool GetPosOffs(glm::dvec3 position, double offset, glm::dvec3 *out) const;

    static constexpr uint8_t kHurtEvent      = 11;
    static constexpr uint8_t kLitEvent       = 12;
    static constexpr uint8_t kUnlitEvent     = 13;
    static constexpr uint8_t kPrimeEvent     = 14;
    static constexpr double kClientBoxOffset = 0.35;
    static const eENTITY_TYPE s_type         = eEntityType_Minecart;

private:
    static bool IsDescending(const glm::dvec3 &direction, int shape);

    BlockPos GetRailPos() const;
    double GetMaxSpeed() const;
    void AdjustToRails(const BlockPos &pos);
    void SetRailYaw(float yaw);
    void MoveAlongTrack();
    double StepAlongTrack(const BlockPos &pos, int shape, double movementLeft);
    void PushEntity(Entity *entity);
    void SetLit(bool lit);
    void Explode(double speedSq);

    int m_type;
    int m_timeSinceHit;
    int m_hurtDir;
    float m_damage;
    bool m_flipped;
    bool m_onRails;
    bool m_lit;
    int m_fuel;
    int m_fuse;
    double m_pushX;
    double m_pushZ;
    std::deque<NetworkStep> m_networkSteps;
    std::unique_ptr<Container> m_container;
    std::unique_ptr<CompoundContainer> m_compoundChest;
};
