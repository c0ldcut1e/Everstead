#pragma once

#include "../../utils/Random.h"
#include "../../world/item/ItemInstance.h"
#include "../Entity.h"

class ItemEntity : public Entity
{
public:
    explicit ItemEntity(Level *level);
    ~ItemEntity() override;

    eENTITY_TYPE GetType() override;
    void Tick() override;
    int GetUpdateInterval() const override;
    bool SendsVelocity() const override;
    bool IsPickable() const override;
    void Load(const CompoundTag *tag) override;
    void Save(CompoundTag *tag) const override;

    void Sync(const glm::dvec3 &position, const glm::dvec3 &velocity, const ItemInstance &item, int age, int pickupDelay);

    ItemInstance &GetItem();
    const ItemInstance &GetItem() const;
    void SetItem(const ItemInstance &item);
    int GetAge() const;
    void SetAge(int age);
    int GetPickupDelay() const;
    void SetPickupDelay(int pickupDelay);
    float GetBobOffset() const;
    bool TakeLanded();
    void StartCollect(const ItemEntity *source, uint8_t playerIndex, int count);
    bool IsCollecting() const;
    uint8_t GetCollectorIndex() const;
    float GetCollectProgress(float partialTicks) const;

    static const eENTITY_TYPE s_type = eEntityType_Item;
    static const int s_lifetime      = 6000;
    static const int s_collectTicks  = 3;

private:
    bool IsNormalCube(int x, int y, int z) const;
    bool PushOutOfBlocks(double x, double y, double z);

    ItemInstance m_item;
    int m_age;
    int m_pickupDelay;
    float m_bobOffset;
    bool m_landed;
    bool m_collecting;
    uint8_t m_collectorIndex;
    int m_collectAge;
};
