#pragma once

#include <string>
#include <vector>

#include "../tag/CompoundTag.h"
#include "../utils/AABB.h"
#include "../utils/UniqueID.h"
#include "../world/Level.h"
#include "../world/models/Model.h"
#include "EntityType.h"
#include "attribute/AttributeMap.h"

#include <glm/vec3.hpp>

enum eENTITY_FLAG : uint8_t
{
    eEntityFlag_Sprinting = 1,
    eEntityFlag_Sneaking  = 2,
    eEntityFlag_Operator  = 4,
    eEntityFlag_OnFire    = 8,
    eEntityFlag_Spectator = 16,
    eEntityFlag_UsingItem = 32,
};

class Entity
{
public:
    static constexpr uint8_t kAfterimageEvent = 30;

    explicit Entity(Level *level);
    virtual ~Entity();

    virtual double GetStepHeight() const;
    bool BlocksBuilding() const;
    AttributeMap *GetAttributes();
    double GetAttributeValue(const Attribute &attribute) const;

    virtual eENTITY_TYPE GetType();

    virtual void Tick();
    virtual bool IsPickable() const;
    virtual bool IsSolid() const;
    virtual double GetRenderDistanceScale() const;
    virtual int GetUpdateInterval() const;
    virtual bool SendsVelocity() const;
    virtual void ReadSpawnData(int data);
    virtual void OnRemoved();
    virtual int GetSpawnData();
    virtual void Load(const CompoundTag *tag);
    virtual void Save(CompoundTag *tag) const;
    virtual void HandleEntityEvent(uint8_t event);

    int GetId() const;
    void SetId(int id);
    int GetTotalTickCount() const;

    Level *GetLevel() const;
    void SetLevel(Level *level);

    void SetPosition(const glm::dvec3 &position);
    void Move(double dx, double dy, double dz);
    void SetRotation(float yaw, float pitch);
    const glm::dvec3 &GetVelocity() const;
    void SetVelocity(const glm::dvec3 &velocity);
    void Push(Entity *other);
    const glm::dvec3 &GetPosition() const;
    const glm::dvec3 &GetOldPosition() const;
    glm::dvec3 GetRenderPosition(float partialTicks) const;
    float GetWalkDist() const;
    float GetWalkDistOld() const;

    const glm::dvec3 &GetFront() const;
    float GetYaw() const;
    float GetPitch() const;
    float GetOldYaw() const;
    float GetOldPitch() const;
    float GetRenderYaw(float partialTicks) const;
    float GetRenderPitch(float partialTicks) const;

    void StoreOld();

    void SetMoveInput(double strafe, double forward);
    void SetJumping(bool value);
    void SetSneaking(bool value);
    bool IsSneaking() const;

    void SetNoGravity(bool value);
    void SetNoCollision(bool value);
    void SetFlying(bool value);

    bool HasNoGravity() const;
    bool HasNoCollision() const;
    bool IsOnGround() const;
    void SetSprinting(bool value);
    bool IsSprinting() const;

    void SetSharedFlag(uint8_t flag, bool value);
    bool GetSharedFlag(uint8_t flag) const;
    void SetSharedFlags(uint8_t flags);
    uint8_t GetSharedFlags() const;

    const AABB &GetBoundingBox() const;
    const AABB &GetAABB() const;

    bool IsInWater() const;
    virtual bool IsOnLadder() const;
    double GetGroundSlipperiness() const;
    bool IsInLava() const;
    bool IsTouchingBlock(const Block *block, int property = -1, int minValue = 0) const;
    bool IsInLiquid(bool lava) const;
    double GetLiquidDepth(bool lava) const;
    void FloatInLiquid(double drag);
    void ApplyWaterFlow();

    const UniqueID &GetUniqueID() const;
    void SetUniqueID(const UniqueID &uniqueId);

    virtual void SetName(const std::wstring &name);
    const std::wstring &GetName() const;
    virtual std::string GetScoreboardName() const;
    PlayerTeam *GetTeam();
    nlohmann::json GetDisplayName();

    void SetModel(Model *model);
    Model *GetModel() const;

    virtual void ApplyNetworkState(const glm::dvec3 &position, float yaw, float pitch);
    virtual void ApplyMoveEntity(int xa, int ya, int za, bool hasPosition, int yaw, int pitch, bool hasRotation, bool onGround);
    virtual void ApplyTeleportEntity(int64_t x, int64_t y, int64_t z, int yaw, int pitch, bool onGround);
    void SetGrounded(bool grounded);
    virtual float GetHeadYaw() const;
    virtual bool ConsumeGroundContact();

    bool StartRiding(Entity *vehicle);
    void StopRiding();
    Entity *GetVehicle() const;
    const std::vector<Entity *> &GetPassengers() const;
    virtual bool CanAddPassenger(Entity *passenger) const;
    virtual void PositionRider(Entity *passenger);
    virtual glm::dvec3 GetDismountPosition(Entity *passenger) const;
    virtual bool IsControlledByRider() const;

    static const eENTITY_TYPE s_type = eEntityType_Entity;

protected:
    void UpdateVectors();
    void UpdateAABB();
    virtual void Travel();
    virtual double GetMovementSpeed() const;
    virtual double GetWaterSpeed() const;
    void MoveRelative(double strafe, double forward, double speed);
    void MoveCollide(double dx, double dy, double dz);
    void JumpFromGround();
    virtual void TickRemote();

    static const int s_lerpSteps;

    Level *m_level;
    int m_id;

    int m_totalTickCount;

    glm::dvec3 m_position;
    glm::dvec3 m_oldPosition;

    glm::dvec3 m_velocity;
    double m_moveStrafe;
    double m_moveForward;

    glm::dvec3 m_front;
    glm::dvec3 m_right;
    glm::dvec3 m_up;

    float m_yaw;
    float m_pitch;
    float m_oldYaw;
    float m_oldPitch;

    double m_airSpeed;
    AttributeMap m_attributes;
    float m_walkDist;
    float m_walkDistOld;

    bool m_onGround;
    bool m_jumping;
    int m_noJumpDelay;
    uint8_t m_sharedFlags;
    bool m_horizontalCollision;
    bool m_verticalCollision;
    std::vector<AABB> m_collisionBoxes;

    bool m_noGravity;
    bool m_noCollision;
    double m_liquidInsetY;
    double m_stepHeight;
    bool m_blocksBuilding;
    bool m_flying;

    AABB m_boundingBox;
    AABB m_aabb;

    UniqueID m_uniqueID;
    std::wstring m_name;

    Model *m_model;

    bool m_hasNetworkState;
    int m_lerpSteps;
    glm::dvec3 m_targetPosition;
    float m_targetYaw;
    float m_targetPitch;
    bool m_grounded;
    int64_t m_xp;
    int64_t m_yp;
    int64_t m_zp;
    int m_yawp;
    int m_pitchp;

    Entity *m_vehicle;
    std::vector<Entity *> m_passengers;
};
