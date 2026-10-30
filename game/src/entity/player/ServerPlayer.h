#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../../utils/Random.h"
#include "../../world/fracture/FractureTrait.h"
#include "../../world/inventory/Container.h"
#include "Player.h"

class EversteadServer;
class ItemEntity;
class PlayerConnection;
class ServerContainerMenu;
class ServerPlayerGameMode;

class ServerPlayer : public Player
{
public:
    ServerPlayer(EversteadServer *server, Level *level, const std::wstring &name, uint8_t playerIndex);
    ~ServerPlayer() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;

    void SetPlayerConnection(PlayerConnection *connection);
    PlayerConnection *GetPlayerConnection() const;
    EversteadServer *GetServer() const;
    ServerPlayerGameMode *GetGameMode() const;
    ServerContainerMenu *GetContainerMenu() const;

    void SetPid(uint32_t pid);
    uint32_t GetPid() const;
    void SetOperator(bool value);
    bool IsOperator() const;

    void UpdateFallDamage(double moveY, bool onGround);
    void CauseMovementExhaustion(double dx, double dy, double dz, bool wasOnGround, bool onGround);
    bool CanReach(const BlockPos &pos) const;
    bool ConsumeGroundContact() override;
    float GetHeadYaw() const override;
    bool Hurt(eDAMAGE_SOURCE source, int damage, LivingEntity *attacker) override;
    void Heal(int amount) override;
    void Respawn();
    void TeleportToSpawn(eDIMENSION_ID dimension);
    void TrySleep(const BlockPos &pos);
    void WakeUp();
    void DismountVehicle();
    void Attack(LivingEntity *target);
    void OnSwing();
    FractureTrait::State *GetFractureState(FractureTrait *trait);
    std::vector<std::pair<FractureTrait *, int>> GetArmorFractures();
    void Knockback(const glm::dvec3 &source, const glm::dvec3 &push) override;
    void SendHealth();
    void StopUsingItem() override;
    void GiveExperience(int amount);
    void PickupExperience(int amount);
    void GiveExperienceLevels(int levels);
    void SendExperience();
    int GetExperienceCooldown() const;
    void SetExperienceCooldown(int ticks);
    void CauseFoodExhaustion(float amount);
    void DropItem(const ItemInstance &item);
    ItemEntity *ThrowItem(const ItemInstance &item);
    void SendArmor(int slot);

    static const eENTITY_TYPE s_type = eEntityType_ServerPlayer;

private:
    void TickFire() override;
    void TickSteppedBlocks();
    void TickTear();
    void TickFractures();
    int AbsorbDamage(eDAMAGE_SOURCE source, int damage) override;
    void OnHurt(eDAMAGE_SOURCE source) override;
    void Die(eDAMAGE_SOURCE source, LivingEntity *attacker) override;
    void CompleteUsingItem() override;
    void CauseDamageExhaustion(eDAMAGE_SOURCE source);
    void SendStatusMessage(const std::string &key);

    EversteadServer *m_server;
    uint32_t m_pid;
    PlayerConnection *m_connection;
    ServerPlayerGameMode *m_gameMode;
    ServerContainerMenu *m_containerMenu;
    bool m_groundContact;
    int m_lastSentHealth;
    int m_lastSentFoodLevel;
    float m_lastSentSaturationLevel;
    int m_experienceCooldown;
    int m_lastLevelUpSoundTick;
    bool m_inTear;
    int m_swingsSinceHit;
    std::unordered_map<FractureTrait *, std::unique_ptr<FractureTrait::State>> m_fractureStates;
    ItemInstance m_lastSentHeldItem;
};
