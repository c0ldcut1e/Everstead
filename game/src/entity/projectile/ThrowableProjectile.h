#pragma once

#include "../Entity.h"

class Item;

class ThrowableProjectile : public Entity
{
public:
    explicit ThrowableProjectile(Level *level);

    void Tick() override;
    int GetUpdateInterval() const override;
    bool SendsVelocity() const override;
    bool IsPickable() const override;
    double GetRenderDistanceScale() const override;
    void ReadSpawnData(int data) override;
    int GetSpawnData() override;

    void Shoot(Entity *owner);
    int GetOwnerId() const;
    void RemoveAfterImpact();
    virtual Item *GetItem() const = 0;

protected:
    virtual void OnHit(Entity *entity) = 0;

    int m_ownerId;
    int m_removeTicks;
};
