#pragma once

#include "Animal.h"

class EatTileGoal;

class Sheep : public Animal
{
public:
    explicit Sheep(Level *level);

    eENTITY_TYPE GetType() override;
    bool Interact(Player *player) override;
    std::unique_ptr<AgeableMob> CreateOffspring(AgeableMob *partner) override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;
    void GetMobData(MobData *data) const override;
    void ApplyMobData(const MobData &data) override;
    void HandleEntityEvent(uint8_t event) override;

    int GetColor() const;
    bool IsSheared() const;
    bool IsShearable() const;
    void Shear();
    void Ate();
    float GetHeadEatPositionScale(float partialTicks) const;
    float GetHeadEatAngleScale(float partialTicks) const;

    static const eENTITY_TYPE s_type        = eEntityType_Sheep;
    static constexpr int kEatAnimationTicks = 40;
    static constexpr uint8_t kEatGrassEvent = 10;

protected:
    void ClientTick() override;
    std::string GetAmbientSound() const override;
    std::string GetHurtSound() const override;
    std::string GetDeathSound() const override;
    void GetLootProperties(std::vector<LootProperty> *properties) const override;

private:
    Item *GetWoolItem() const;

    int m_color;
    bool m_sheared;
    int m_eatAnimationTick;
};
