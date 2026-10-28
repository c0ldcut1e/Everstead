#pragma once

#include <cstdint>
#include <memory>

#include "Entity.h"

class EntityTracker;
class Packet;

class TrackedEntity
{
public:
    explicit TrackedEntity(Entity *entity);
    ~TrackedEntity();

    void Tick(EntityTracker *tracker);

    Entity *GetEntity() const;

    static const int s_positionScale;

private:
    void Send(EntityTracker *tracker, std::shared_ptr<Packet> packet) const;
    int64_t PositionToPacked(double value) const;
    int RotationToPacked(float value) const;
    int WrapPackedRotationDelta(int value) const;

    static const int s_resyncInterval;
    static const int s_rotationToleranceLevel;
    static const int s_headToleranceLevel;
    static const int s_rotationScale;
    static const int s_minByteValue;
    static const int s_maxByteValue;
    static const float s_fullRotation;

    Entity *m_entity;
    Entity *m_controller;
    int m_tickCount;
    int64_t m_xp;
    int64_t m_yp;
    int64_t m_zp;
    int m_yawp;
    int m_pitchp;
    int m_headYawp;
    bool m_onGround;
    glm::dvec3 m_velocity;
};
