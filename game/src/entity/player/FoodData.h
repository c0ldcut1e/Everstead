#pragma once

class CompoundTag;
class ServerPlayer;

class FoodData
{
public:
    FoodData();

    void Eat(int nutrition, float saturationModifier);
    void Tick(ServerPlayer *player);
    void AddExhaustion(float amount);
    bool NeedsFood() const;
    void Read(const CompoundTag *tag);
    void Write(CompoundTag *tag) const;

    int GetFoodLevel() const;
    void SetFoodLevel(int foodLevel);
    float GetSaturationLevel() const;
    void SetSaturationLevel(float saturationLevel);

    static constexpr int kMaxFoodLevel           = 20;
    static constexpr float kExhaustionJump       = 0.05f;
    static constexpr float kExhaustionSprintJump = 0.2f;
    static constexpr float kExhaustionMine       = 0.005f;
    static constexpr float kExhaustionAttack     = 0.1f;
    static constexpr float kExhaustionSprint     = 0.1f;
    static constexpr float kExhaustionSwim       = 0.01f;

private:
    int m_foodLevel;
    float m_saturationLevel;
    float m_exhaustionLevel;
    int m_tickTimer;
    float m_healProgress;
};
