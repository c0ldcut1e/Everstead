#include "Entity.h"

#include <algorithm>
#include <cmath>
#include <numbers>

#include "../tag/DoubleTag.h"
#include "../tag/FloatTag.h"
#include "../tag/ListTag.h"
#include "../utils/StringUtils.h"
#include "../world/block/Block.h"
#include "../world/block/BlockPos.h"
#include "../world/block/IceSheetBlock.h"
#include "../world/block/ScaffoldingBlock.h"
#include "../world/chunk/Chunk.h"
#include "EntityFactory.h"
#include "LivingEntity.h"
#include "TrackedEntity.h"

#include <glm/gtx/norm.hpp>

static const double s_gravityDrag       = 0.98;
static const double s_airFriction       = 0.91;
static const float s_groundSlipperiness = 0.6f;
static const double s_groundSpeedFactor = 0.16277136;
static const double s_sprintJumpBoost   = 0.2;
static const double s_sprintMultiplier  = 1.3;
static const double s_flyVerticalDrag   = 0.6;
static const double s_sneakEdgeStep     = 0.05;
static const double s_waterSpeed        = 0.02;
static const double s_waterFriction     = 0.8;
static const double s_waterSink         = 0.02;
static const double s_lavaSpeed         = 0.02;
static const double s_lavaFriction      = 0.5;
static const double s_waterJumpPower    = 0.04;
static const double s_waterLedgeBoost   = 0.3;
static const double s_waterLedgeReach   = 0.6;
static const double s_waterCheckInsetY  = 0.401;
static const double s_waterCheckInset   = 0.001;
static const double s_ladderSpeed       = 0.15;
static const double s_ladderClimbSpeed  = 0.2;

// We ignore boxes the entity already overlaps on this axis so an entity stuck inside a block can still move out
static double ClipAxis(const AABB &entity, const std::vector<AABB> &boxes, int axis, double delta)
{
    // Rebuilding the AABB from the position can leave it slightly inside a face it was clipped to
    constexpr double kEpsilon = 1e-7;
    const glm::dvec3 &min     = entity.GetMin();
    const glm::dvec3 &max     = entity.GetMax();
    for (size_t i = 0; i < boxes.size() && delta != 0.0; i++)
    {
        const glm::dvec3 &boxMin = boxes[i].GetMin();
        const glm::dvec3 &boxMax = boxes[i].GetMax();
        bool overlapX            = max.x > boxMin.x + kEpsilon && min.x < boxMax.x - kEpsilon;
        bool overlapY            = max.y > boxMin.y + kEpsilon && min.y < boxMax.y - kEpsilon;
        bool overlapZ            = max.z > boxMin.z + kEpsilon && min.z < boxMax.z - kEpsilon;
        double entityMin         = axis == 0 ? min.x : (axis == 1 ? min.y : min.z);
        double entityMax         = axis == 0 ? max.x : (axis == 1 ? max.y : max.z);
        double blockMin          = axis == 0 ? boxMin.x : (axis == 1 ? boxMin.y : boxMin.z);
        double blockMax          = axis == 0 ? boxMax.x : (axis == 1 ? boxMax.y : boxMax.z);
        bool overlapOther        = axis == 0 ? (overlapY && overlapZ) : (axis == 1 ? (overlapX && overlapZ) : (overlapX && overlapY));
        if (!overlapOther)
        {
            continue;
        }

        if (delta > 0.0 && entityMax <= blockMin + kEpsilon)
        {
            delta = std::min(delta, std::max(blockMin - entityMax, 0.0));
        }
        else if (delta < 0.0 && entityMin >= blockMax - kEpsilon)
        {
            delta = std::max(delta, std::min(blockMax - entityMin, 0.0));
        }
    }

    return delta;
}

static double ReduceTowardZero(double value, double step)
{
    if (value < step && value >= -step)
    {
        return 0.0;
    }

    if (value > 0.0)
    {
        return value - step;
    }

    return value + step;
}

const int Entity::s_lerpSteps = 3;

Entity::Entity(Level *level)
{
    m_level               = level;
    m_id                  = 0;
    m_totalTickCount      = -1;
    m_position            = {0.0, 0.0, 0.0};
    m_oldPosition         = m_position;
    m_velocity            = {0.0, 0.0, 0.0};
    m_moveStrafe          = 0.0;
    m_moveForward         = 0.0;
    m_front               = {0.0, 0.0, -1.0};
    m_up                  = {0.0, 1.0, 0.0};
    m_yaw                 = -90.0f;
    m_pitch               = 0.0f;
    m_oldYaw              = m_yaw;
    m_oldPitch            = m_pitch;
    m_airSpeed            = 0.02;
    m_walkDist            = 0.0f;
    m_walkDistOld         = 0.0f;
    m_onGround            = false;
    m_jumping             = false;
    m_noJumpDelay         = 0;
    m_sharedFlags         = 0;
    m_horizontalCollision = false;
    m_verticalCollision   = false;
    m_collisionBoxes      = {};
    m_noGravity           = false;
    m_noCollision         = false;
    m_liquidInsetY        = s_waterCheckInsetY;
    m_stepHeight          = 0.0;
    m_blocksBuilding      = false;
    m_flying              = false;
    m_boundingBox         = {glm::dvec3(-0.3, 0.0, -0.3), glm::dvec3(0.3, 1.8, 0.3)};
    m_aabb                = {};
    m_uniqueID            = UniqueID::Random();
    m_name                = L"";
    m_model               = nullptr;
    m_hasNetworkState     = false;
    m_lerpSteps           = 0;
    m_targetPosition      = glm::dvec3();
    m_targetYaw           = 0.0f;
    m_targetPitch         = 0.0f;
    m_grounded            = true;
    m_xp                  = 0;
    m_yp                  = 0;
    m_zp                  = 0;
    m_yawp                = 0;
    m_pitchp              = 0;
    m_vehicle             = nullptr;
}

Entity::~Entity()
{
    StopRiding();
    for (size_t i = 0; i < m_passengers.size(); i++)
    {
        m_passengers[i]->m_vehicle = nullptr;
    }
}

eENTITY_TYPE Entity::GetType()
{
    return s_type;
}

bool Entity::IsPickable() const
{
    return !GetSharedFlag(eEntityFlag_Spectator);
}

bool Entity::IsSolid() const
{
    return false;
}

double Entity::GetRenderDistanceScale() const
{
    return 1.0;
}

int Entity::GetUpdateInterval() const
{
    return 3;
}

bool Entity::SendsVelocity() const
{
    return false;
}

void Entity::OnRemoved()
{}

void Entity::Tick()
{
    m_totalTickCount++;

    UpdateVectors();
    UpdateAABB();

    Travel();

    m_moveStrafe  = 0.0;
    m_moveForward = 0.0;
}

void Entity::ReadSpawnData(int data)
{
    (void) data;
}

int Entity::GetSpawnData()
{
    return 0;
}

void Entity::Load(const CompoundTag *tag)
{
    std::vector<int> uniqueId = tag->GetIntArray("UID");
    if (uniqueId.size() == 5)
    {
        SetUniqueID(UniqueID((uint32_t) uniqueId[0], (uint32_t) uniqueId[1], (uint32_t) uniqueId[2], (uint32_t) uniqueId[3], (uint32_t) uniqueId[4]));
    }

    if (tag->Contains("Name", Tag::eTagType_String))
    {
        SetName(StringUtils::ToUtf16(tag->GetString("Name")));
    }

    ListTag *position = tag->GetList("Pos");
    if (position && position->Size() == 3)
    {
        SetPosition(glm::dvec3(position->GetNumber(0), position->GetNumber(1), position->GetNumber(2)));
    }

    ListTag *motion = tag->GetList("Motion");
    if (motion && motion->Size() == 3 && std::isfinite(motion->GetNumber(0)) && std::isfinite(motion->GetNumber(1)) &&
        std::isfinite(motion->GetNumber(2)))
    {
        SetVelocity(glm::dvec3(motion->GetNumber(0), motion->GetNumber(1), motion->GetNumber(2)));
    }

    ListTag *rotation = tag->GetList("Rotation");
    if (rotation && rotation->Size() == 2 && std::isfinite(rotation->GetNumber(0)) && std::isfinite(rotation->GetNumber(1)))
    {
        SetRotation((float) rotation->GetNumber(0), (float) rotation->GetNumber(1));
    }

    if (tag->Contains("OnGround"))
    {
        m_onGround = tag->GetBoolean("OnGround");
    }

    if (tag->Contains("NoGravity"))
    {
        m_noGravity = tag->GetBoolean("NoGravity");
    }
}

void Entity::Save(CompoundTag *tag) const
{
    const uint32_t *uniqueId = m_uniqueID.GetData();
    tag->PutIntArray("UID", {(int) uniqueId[0], (int) uniqueId[1], (int) uniqueId[2], (int) uniqueId[3], (int) uniqueId[4]});
    tag->PutString("Name", StringUtils::ToUtf8(m_name));

    std::unique_ptr<ListTag> position(new ListTag());
    position->Add(std::unique_ptr<Tag>(new DoubleTag(m_position.x)));
    position->Add(std::unique_ptr<Tag>(new DoubleTag(m_position.y)));
    position->Add(std::unique_ptr<Tag>(new DoubleTag(m_position.z)));
    tag->Put("Pos", std::move(position));

    std::unique_ptr<ListTag> motion(new ListTag());
    motion->Add(std::unique_ptr<Tag>(new DoubleTag(m_velocity.x)));
    motion->Add(std::unique_ptr<Tag>(new DoubleTag(m_velocity.y)));
    motion->Add(std::unique_ptr<Tag>(new DoubleTag(m_velocity.z)));
    tag->Put("Motion", std::move(motion));

    std::unique_ptr<ListTag> rotation(new ListTag());
    rotation->Add(std::unique_ptr<Tag>(new FloatTag(m_yaw)));
    rotation->Add(std::unique_ptr<Tag>(new FloatTag(m_pitch)));
    tag->Put("Rotation", std::move(rotation));

    tag->PutBoolean("OnGround", m_onGround);
    tag->PutBoolean("NoGravity", m_noGravity);
}

void Entity::HandleEntityEvent(uint8_t event)
{
    (void) event;
}

int Entity::GetId() const
{
    return m_id;
}

int Entity::GetTotalTickCount() const
{
    return m_totalTickCount;
}

void Entity::SetId(int id)
{
    m_level->UpdateEntityId(this, m_id, id);
    m_id = id;
}

Level *Entity::GetLevel() const
{
    return m_level;
}

void Entity::SetLevel(Level *level)
{
    m_level = level;
}

void Entity::SetPosition(const glm::dvec3 &position)
{
    m_position = position;
    UpdateAABB();
}

const glm::dvec3 &Entity::GetVelocity() const
{
    return m_velocity;
}

void Entity::SetVelocity(const glm::dvec3 &velocity)
{
    m_velocity = velocity;
}

void Entity::SetRotation(float yaw, float pitch)
{
    m_yaw   = yaw;
    m_pitch = pitch;
    UpdateVectors();
}

const glm::dvec3 &Entity::GetPosition() const
{
    return m_position;
}

const glm::dvec3 &Entity::GetOldPosition() const
{
    return m_oldPosition;
}

glm::dvec3 Entity::GetRenderPosition(float partialTicks) const
{
    const glm::dvec3 &a = m_oldPosition;
    const glm::dvec3 &b = m_position;
    return glm::dvec3(a.x + (b.x - a.x) * partialTicks, a.y + (b.y - a.y) * partialTicks, a.z + (b.z - a.z) * partialTicks);
}

float Entity::GetWalkDist() const
{
    return m_walkDist;
}

float Entity::GetWalkDistOld() const
{
    return m_walkDistOld;
}

const glm::dvec3 &Entity::GetFront() const
{
    return m_front;
}

float Entity::GetYaw() const
{
    return m_yaw;
}

float Entity::GetPitch() const
{
    return m_pitch;
}

float Entity::GetOldYaw() const
{
    return m_oldYaw;
}

float Entity::GetOldPitch() const
{
    return m_oldPitch;
}

float Entity::GetRenderYaw(float partialTicks) const
{
    float d = m_yaw - m_oldYaw;
    while (d <= -180.0f)
    {
        d += 360.0f;
    }

    while (d > 180.0f)
    {
        d -= 360.0f;
    }

    return m_oldYaw + d * partialTicks;
}

float Entity::GetRenderPitch(float partialTicks) const
{
    return m_oldPitch + (m_pitch - m_oldPitch) * partialTicks;
}

void Entity::StoreOld()
{
    m_oldPosition = m_position;
    m_oldYaw      = m_yaw;
    m_oldPitch    = m_pitch;
    m_walkDistOld = m_walkDist;
}

void Entity::SetMoveInput(double strafe, double forward)
{
    m_moveStrafe  = strafe;
    m_moveForward = forward;
}

void Entity::SetJumping(bool value)
{
    m_jumping = value;
}

void Entity::SetSneaking(bool value)
{
    SetSharedFlag(eEntityFlag_Sneaking, value);
}

bool Entity::IsSneaking() const
{
    return GetSharedFlag(eEntityFlag_Sneaking);
}

void Entity::SetNoGravity(bool value)
{
    m_noGravity = value;
}

void Entity::SetNoCollision(bool value)
{
    m_noCollision = value;
}

void Entity::SetFlying(bool value)
{
    m_flying = value;
    if (m_flying)
    {
        m_velocity    = glm::dvec3(0.0, 0.0, 0.0);
        m_noGravity   = true;
        m_noCollision = false;
    }
    else
    {
        m_noGravity   = false;
        m_noCollision = false;
    }
}

bool Entity::HasNoGravity() const
{
    return m_noGravity;
}

bool Entity::HasNoCollision() const
{
    return m_noCollision;
}

bool Entity::IsOnGround() const
{
    return m_onGround;
}

void Entity::SetSprinting(bool value)
{
    SetSharedFlag(eEntityFlag_Sprinting, value);
}

bool Entity::IsSprinting() const
{
    return GetSharedFlag(eEntityFlag_Sprinting);
}

void Entity::SetSharedFlag(uint8_t flag, bool value)
{
    if (value)
    {
        m_sharedFlags |= flag;
    }
    else
    {
        m_sharedFlags &= (uint8_t) ~flag;
    }
}

bool Entity::GetSharedFlag(uint8_t flag) const
{
    return (m_sharedFlags & flag) != 0;
}

void Entity::SetSharedFlags(uint8_t flags)
{
    m_sharedFlags = flags;
}

uint8_t Entity::GetSharedFlags() const
{
    return m_sharedFlags;
}

const AABB &Entity::GetBoundingBox() const
{
    return m_boundingBox;
}

const AABB &Entity::GetAABB() const
{
    return m_aabb;
}

const UniqueID &Entity::GetUniqueID() const
{
    return m_uniqueID;
}

void Entity::SetUniqueID(const UniqueID &uniqueId)
{
    m_uniqueID = uniqueId;
}

void Entity::SetName(const std::wstring &name)
{
    m_name = name;
}

const std::wstring &Entity::GetName() const
{
    return m_name;
}

std::string Entity::GetScoreboardName() const
{
    return m_uniqueID.ToString();
}

PlayerTeam *Entity::GetTeam()
{
    return m_level->GetScoreboard().GetMemberTeam(GetScoreboardName());
}

nlohmann::json Entity::GetDisplayName()
{
    nlohmann::json name = m_name.empty()
                                  ? nlohmann::json{{"translate", "entity.everstead." + EntityFactory::GetResourceLocation(GetType()).GetPath()}}
                                  : nlohmann::json{{"text", StringUtils::ToUtf8(m_name)}};
    PlayerTeam *team    = GetTeam();
    return team ? team->FormatName(name) : name;
}

void Entity::SetModel(Model *model)
{
    m_model = model;
}

Model *Entity::GetModel() const
{
    return m_model;
}

void Entity::ApplyNetworkState(const glm::dvec3 &position, float yaw, float pitch)
{
    m_xp             = (int64_t) floor(position.x * TrackedEntity::s_positionScale);
    m_yp             = (int64_t) floor(position.y * TrackedEntity::s_positionScale);
    m_zp             = (int64_t) floor(position.z * TrackedEntity::s_positionScale);
    m_yawp           = (int) floor(yaw * 256.0f / 360.0f);
    m_pitchp         = (int) floor(pitch * 256.0f / 360.0f);
    m_targetPosition = position;
    m_targetYaw      = yaw;
    m_targetPitch    = pitch;
    m_lerpSteps      = s_lerpSteps;
    if (!m_hasNetworkState)
    {
        m_hasNetworkState = true;
        m_lerpSteps       = 0;
        SetPosition(position);
        SetRotation(yaw, pitch);
        StoreOld();
        UpdateAABB();
    }
    else if (SendsVelocity())
    {
        m_lerpSteps = 0;
        SetPosition(position);
        SetRotation(yaw, pitch);
        UpdateAABB();
    }
}

void Entity::ApplyMoveEntity(int xa, int ya, int za, bool hasPosition, int yaw, int pitch, bool hasRotation, bool onGround)
{
    if (hasPosition)
    {
        m_xp += xa;
        m_yp += ya;
        m_zp += za;
        m_grounded = onGround;
    }

    if (hasRotation)
    {
        m_yawp += yaw;
        m_pitchp += pitch;
    }

    ApplyNetworkState(glm::dvec3((double) m_xp / TrackedEntity::s_positionScale, (double) m_yp / TrackedEntity::s_positionScale,
                                 (double) m_zp / TrackedEntity::s_positionScale),
                      (float) m_yawp * 360.0f / 256.0f, (float) m_pitchp * 360.0f / 256.0f);
}

void Entity::ApplyTeleportEntity(int64_t x, int64_t y, int64_t z, int yaw, int pitch, bool onGround)
{
    m_grounded = onGround;
    m_xp       = x;
    m_yp       = y;
    m_zp       = z;
    m_yawp     = yaw;
    m_pitchp   = pitch;
    ApplyNetworkState(glm::dvec3((double) m_xp / TrackedEntity::s_positionScale, (double) m_yp / TrackedEntity::s_positionScale,
                                 (double) m_zp / TrackedEntity::s_positionScale),
                      (float) m_yawp * 360.0f / 256.0f, (float) m_pitchp * 360.0f / 256.0f);
}

void Entity::SetGrounded(bool grounded)
{
    m_grounded = grounded;
}

float Entity::GetHeadYaw() const
{
    return m_yaw;
}

bool Entity::ConsumeGroundContact()
{
    return m_onGround;
}

void Entity::TickRemote()
{
    if (m_lerpSteps > 0)
    {
        glm::dvec3 position = GetPosition();
        position.x += (m_targetPosition.x - position.x) / (double) m_lerpSteps;
        position.y += (m_targetPosition.y - position.y) / (double) m_lerpSteps;
        position.z += (m_targetPosition.z - position.z) / (double) m_lerpSteps;

        float yawDelta = m_targetYaw - m_yaw;
        while (yawDelta <= -180.0f)
        {
            yawDelta += 360.0f;
        }

        while (yawDelta > 180.0f)
        {
            yawDelta -= 360.0f;
        }

        float pitchDelta = m_targetPitch - m_pitch;
        float yaw        = m_yaw + yawDelta / (float) m_lerpSteps;
        float pitch      = m_pitch + pitchDelta / (float) m_lerpSteps;

        SetPosition(position);
        SetRotation(yaw, pitch);
        m_lerpSteps--;
    }

    UpdateAABB();
}

bool Entity::StartRiding(Entity *vehicle)
{
    if (m_vehicle == vehicle)
    {
        return true;
    }

    if (!vehicle->CanAddPassenger(this))
    {
        return false;
    }

    StopRiding();
    m_vehicle = vehicle;
    vehicle->m_passengers.push_back(this);
    return true;
}

void Entity::StopRiding()
{
    if (!m_vehicle)
    {
        return;
    }

    std::erase(m_vehicle->m_passengers, this);
    m_vehicle = nullptr;
}

Entity *Entity::GetVehicle() const
{
    return m_vehicle;
}

const std::vector<Entity *> &Entity::GetPassengers() const
{
    return m_passengers;
}

bool Entity::CanAddPassenger(Entity *passenger) const
{
    (void) passenger;
    return false;
}

void Entity::PositionRider(Entity *passenger)
{
    passenger->SetPosition(m_position);
}

glm::dvec3 Entity::GetDismountPosition(Entity *passenger) const
{
    return glm::dvec3(passenger->GetPosition().x, GetAABB().GetMax().y, passenger->GetPosition().z);
}

bool Entity::IsControlledByRider() const
{
    return false;
}

void Entity::UpdateVectors()
{
    float yawRad   = m_yaw * (float) (std::numbers::pi / 180.0);
    float pitchRad = m_pitch * (float) (std::numbers::pi / 180.0);

    glm::dvec3 front(cos(yawRad) * cos(pitchRad), sin(pitchRad), sin(yawRad) * cos(pitchRad));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, glm::dvec3(0.0, 1.0, 0.0)));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

void Entity::UpdateAABB()
{
    m_aabb = m_boundingBox.Translated(m_position);
}

void Entity::Push(Entity *other)
{
    double dx       = other->m_position.x - m_position.x;
    double dz       = other->m_position.z - m_position.z;
    double distance = std::max(fabs(dx), fabs(dz));
    if (distance < 0.01)
    {
        return;
    }

    distance     = sqrt(distance);
    double scale = std::min(1.0 / distance, 1.0) * 0.05 / distance;
    dx *= scale;
    dz *= scale;
    if (m_passengers.empty())
    {
        m_velocity += glm::dvec3(-dx, 0.0, -dz);
    }

    if (other->m_passengers.empty())
    {
        other->m_velocity += glm::dvec3(dx, 0.0, dz);
    }
}

void Entity::MoveRelative(double strafe, double forward, double speed)
{
    double dist = strafe * strafe + forward * forward;
    if (dist < 1.0e-4)
    {
        return;
    }

    dist = sqrt(dist);
    if (dist < 1.0)
    {
        dist = 1.0;
    }

    dist = speed / dist;
    strafe *= dist;
    forward *= dist;

    double yawRad = m_yaw * (std::numbers::pi / 180.0);
    double sinYaw = sin(yawRad);
    double cosYaw = cos(yawRad);

    m_velocity.x += forward * cosYaw - strafe * sinYaw;
    m_velocity.z += forward * sinYaw + strafe * cosYaw;
}

void Entity::JumpFromGround()
{
    m_velocity.y = GetAttributeValue(Attribute::s_jumpStrength);
    if (IsSprinting())
    {
        double yawRad = m_yaw * (std::numbers::pi / 180.0);
        m_velocity.x += cos(yawRad) * s_sprintJumpBoost;
        m_velocity.z += sin(yawRad) * s_sprintJumpBoost;
    }
}

double Entity::GetStepHeight() const
{
    return m_stepHeight;
}

bool Entity::BlocksBuilding() const
{
    return m_blocksBuilding;
}

AttributeMap *Entity::GetAttributes()
{
    return &m_attributes;
}

double Entity::GetAttributeValue(const Attribute &attribute) const
{
    return m_attributes.GetValue(attribute);
}

bool Entity::IsInWater() const
{
    return IsInLiquid(false);
}

bool Entity::IsTouchingBlock(const Block *block, int property, int minValue) const
{
    const glm::dvec3 &min = m_aabb.GetMin();
    const glm::dvec3 &max = m_aabb.GetMax();
    for (int y = std::max((int) floor(min.y + 0.001), 0); y <= std::min((int) floor(max.y - 0.001), Chunk::s_sizeY - 1); y++)
    {
        for (int x = (int) floor(min.x + 0.001); x <= (int) floor(max.x - 0.001); x++)
        {
            for (int z = (int) floor(min.z + 0.001); z <= (int) floor(max.z - 0.001); z++)
            {
                BlockPos pos(x, y, z);
                if (Block::ById(m_level->GetBlockId(pos)) == block &&
                    (property < 0 || block->GetPropertyValue(m_level->GetBlockData(pos), property) >= minValue))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool Entity::IsInLava() const
{
    return IsInLiquid(true);
}

bool Entity::IsInLiquid(bool lava) const
{
    glm::dvec3 min = m_aabb.GetMin() + glm::dvec3(s_waterCheckInset, m_liquidInsetY, s_waterCheckInset);
    glm::dvec3 max = m_aabb.GetMax() + glm::dvec3(-s_waterCheckInset, -m_liquidInsetY, -s_waterCheckInset);
    int maxY       = (int) floor(max.y + 1.0);

    for (int y = (int) floor(min.y); y < maxY; y++)
    {
        for (int x = (int) floor(min.x); x < (int) floor(max.x + 1.0); x++)
        {
            for (int z = (int) floor(min.z); z < (int) floor(max.z + 1.0); z++)
            {
                BlockPos pos(x, y, z);
                Block *block = Block::ById(m_level->GetBlockId(pos));
                if (block && block->IsLiquid() && block->IsLava() == lava &&
                    (double) maxY >= (double) y + block->GetLiquidHeight(m_level->GetBlockData(pos)))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

double Entity::GetLiquidDepth(bool lava) const
{
    double depth = 0.0;
    for (int y = (int) floor(m_aabb.GetMin().y); y < (int) floor(m_aabb.GetMax().y + 1.0); y++)
    {
        for (int x = (int) floor(m_aabb.GetMin().x); x < (int) floor(m_aabb.GetMax().x + 1.0); x++)
        {
            for (int z = (int) floor(m_aabb.GetMin().z); z < (int) floor(m_aabb.GetMax().z + 1.0); z++)
            {
                BlockPos pos(x, y, z);
                Block *block = Block::ById(m_level->GetBlockId(pos));
                if (block && block->IsLiquid() && block->IsLava() == lava)
                {
                    depth = std::max(depth, y + block->GetLiquidHeight(m_level->GetBlockData(pos)) - m_aabb.GetMin().y);
                }
            }
        }
    }

    return depth;
}

void Entity::FloatInLiquid(double drag)
{
    m_velocity.x *= drag;
    m_velocity.z *= drag;
    if (m_velocity.y < 0.06)
    {
        m_velocity.y += 5.0e-4;
    }
}

void Entity::ApplyWaterFlow()
{
    glm::dvec3 min = m_aabb.GetMin() + glm::dvec3(s_waterCheckInset, m_liquidInsetY, s_waterCheckInset);
    glm::dvec3 max = m_aabb.GetMax() + glm::dvec3(-s_waterCheckInset, -m_liquidInsetY, -s_waterCheckInset);
    glm::dvec3 current;
    int maxY = (int) floor(max.y + 1.0);
    for (int y = (int) floor(min.y); y < maxY; y++)
    {
        for (int x = (int) floor(min.x); x < (int) floor(max.x + 1.0); x++)
        {
            for (int z = (int) floor(min.z); z < (int) floor(max.z + 1.0); z++)
            {
                BlockPos pos(x, y, z);
                Block *block = Block::ById(m_level->GetBlockId(pos));
                if (block && block->IsLiquid() && !block->IsLava() &&
                    (double) maxY >= (double) y + block->GetLiquidHeight(m_level->GetBlockData(pos)))
                {
                    current += block->GetFlow(m_level, pos);
                }
            }
        }
    }

    if (glm::length2(current) > 0.0)
    {
        m_velocity += glm::normalize(current) * 0.014;
    }
}

bool Entity::IsOnLadder() const
{
    BlockPos pos((int) floor(m_position.x), (int) floor(m_aabb.GetMin().y), (int) floor(m_position.z));
    Block *block = Block::ById(m_level->GetBlockId(pos));
    return block && block->IsClimbable();
}

// This uses the slipperiest block under the whole hitbox, so standing half on ice still slides
double Entity::GetGroundSlipperiness() const
{
    int x = (int) floor(m_position.x);
    int z = (int) floor(m_position.z);

    // Floor ice sheets sit inside the feet block instead of the block below
    BlockPos layerPos(x, (int) floor(m_aabb.GetMin().y), z);
    IceSheetBlock *layer = dynamic_cast<IceSheetBlock *>(Block::ById(m_level->GetBlockId(layerPos)));
    if (layer && layer->HasFace(m_level->GetBlockData(layerPos), Direction::down))
    {
        return layer->GetSlipperiness();
    }

    int y               = (int) floor(m_aabb.GetMin().y - 0.5000001);
    double slipperiness = s_groundSlipperiness;
    for (int bz = (int) floor(m_aabb.GetMin().z); bz <= (int) floor(m_aabb.GetMax().z - 1e-6); bz++)
    {
        for (int bx = (int) floor(m_aabb.GetMin().x); bx <= (int) floor(m_aabb.GetMax().x - 1e-6); bx++)
        {
            slipperiness = std::max(slipperiness, (double) Block::ById(m_level->GetBlockId(BlockPos(bx, y, bz)))->GetSlipperiness());
        }
    }

    return slipperiness;
}

double Entity::GetMovementSpeed() const
{
    return GetAttributeValue(Attribute::s_movementSpeed);
}

double Entity::GetWaterSpeed() const
{
    return s_waterSpeed;
}

void Entity::Travel()
{
    if (m_noJumpDelay > 0)
    {
        m_noJumpDelay--;
    }

    if (!m_jumping)
    {
        m_noJumpDelay = 0;
    }

    if (m_flying)
    {
        int verticalInput  = (m_jumping ? 1 : 0) - (IsSneaking() ? 1 : 0);
        double flyingSpeed = GetAttributeValue(Attribute::s_flyingSpeed);
        m_velocity.y += flyingSpeed * 3.0 * (double) verticalInput;

        double preTravelVerticalVelocity = m_velocity.y;
        MoveRelative(m_moveStrafe, m_moveForward, flyingSpeed);
        Move(m_velocity.x, m_velocity.y, m_velocity.z);
        m_velocity.x *= s_airFriction;
        m_velocity.z *= s_airFriction;
        m_velocity.y = preTravelVerticalVelocity * s_flyVerticalDrag;
        return;
    }

    if (IsInWater())
    {
        ApplyWaterFlow();

        if (m_jumping)
        {
            m_velocity.y += s_waterJumpPower;
        }

        double startY = m_position.y;
        MoveRelative(m_moveStrafe, m_moveForward, GetWaterSpeed());
        Move(m_velocity.x, m_velocity.y, m_velocity.z);

        m_velocity.x *= s_waterFriction;
        m_velocity.y *= s_waterFriction;
        m_velocity.z *= s_waterFriction;
        m_velocity.y -= s_waterSink;

        if (m_horizontalCollision)
        {
            glm::dvec3 ledgeStep(m_velocity.x, m_velocity.y + s_waterLedgeReach - m_position.y + startY, m_velocity.z);
            AABB ledgeBox = m_aabb.Translated(ledgeStep);
            if (!m_level->IntersectsBlock(ledgeBox) && !m_level->ContainsLiquid(ledgeBox))
            {
                m_velocity.y = s_waterLedgeBoost;
            }
        }

        return;
    }

    if (IsInLava())
    {
        if (m_jumping)
        {
            m_velocity.y += s_waterJumpPower;
        }

        double startY = m_position.y;
        MoveRelative(m_moveStrafe, m_moveForward, s_lavaSpeed);
        Move(m_velocity.x, m_velocity.y, m_velocity.z);

        m_velocity.x *= s_lavaFriction;
        m_velocity.y *= s_lavaFriction;
        m_velocity.z *= s_lavaFriction;
        m_velocity.y -= s_waterSink;

        if (m_horizontalCollision)
        {
            glm::dvec3 ledgeStep(m_velocity.x, m_velocity.y + s_waterLedgeReach - m_position.y + startY, m_velocity.z);
            AABB ledgeBox = m_aabb.Translated(ledgeStep);
            if (!m_level->IntersectsBlock(ledgeBox) && !m_level->ContainsLiquid(ledgeBox))
            {
                m_velocity.y = s_waterLedgeBoost;
            }
        }

        return;
    }

    // Holding jump waits 10 ticks between ground jumps instead of hopping every tick it lands
    if (m_jumping && m_onGround && m_noJumpDelay == 0)
    {
        JumpFromGround();
        m_noJumpDelay = 10;
    }

    double friction = s_airFriction;
    if (m_onGround)
    {
        friction = GetGroundSlipperiness() * s_airFriction;
    }

    double speed;
    if (m_onGround)
    {
        speed = GetMovementSpeed() * s_groundSpeedFactor / (friction * friction * friction);
    }
    else
    {
        speed = m_airSpeed;
    }

    if (IsSprinting())
    {
        speed *= s_sprintMultiplier;
    }

    double strafe  = m_moveStrafe;
    double forward = m_moveForward;
    if (IsSneaking())
    {
        double sneakingSpeed = GetAttributeValue(Attribute::s_sneakingSpeed);
        strafe *= sneakingSpeed;
        forward *= sneakingSpeed;
    }

    MoveRelative(strafe, forward, speed);

    bool onLadder = IsOnLadder();
    if (onLadder)
    {
        m_velocity.x = std::clamp(m_velocity.x, -s_ladderSpeed, s_ladderSpeed);
        m_velocity.z = std::clamp(m_velocity.z, -s_ladderSpeed, s_ladderSpeed);
        m_velocity.y = std::max(m_velocity.y, -s_ladderSpeed);
        bool inScaffolding =
                Block::ById(m_level->GetBlockId(BlockPos((int) floor(m_position.x), (int) floor(m_aabb.GetMin().y), (int) floor(m_position.z)))) ==
                Block::ByName("scaffolding");
        if (IsSneaking() && m_velocity.y < 0.0 && !inScaffolding)
        {
            m_velocity.y = 0.0;
        }
    }

    Move(m_velocity.x, m_velocity.y, m_velocity.z);

    if (onLadder && (m_horizontalCollision || m_jumping))
    {
        m_velocity.y = s_ladderClimbSpeed;
    }

    m_velocity.y -= GetAttributeValue(Attribute::s_gravity);
    m_velocity.y *= s_gravityDrag;
    m_velocity.x *= friction;
    m_velocity.z *= friction;
}

void Entity::MoveCollide(double dx, double dy, double dz)
{
    glm::dvec3 min = m_aabb.GetMin();
    glm::dvec3 max = m_aabb.GetMax();
    AABB sweep(glm::dvec3(min.x + std::min(dx, 0.0), min.y + std::min(dy, 0.0), min.z + std::min(dz, 0.0)),
               glm::dvec3(max.x + std::max(dx, 0.0), max.y + std::max(dy, 0.0), max.z + std::max(dz, 0.0)));
    m_level->GetCollisionBoxes(sweep, &m_collisionBoxes);
    m_level->GetWorldBorder().GetCollisionBoxes(m_position, sweep, &m_collisionBoxes);
    ScaffoldingBlock *scaffolding = (ScaffoldingBlock *) Block::ByName("scaffolding");
    for (int x = (int) floor(sweep.GetMin().x); x <= (int) floor(sweep.GetMax().x); x++)
    {
        for (int y = (int) floor(sweep.GetMin().y); y <= (int) floor(sweep.GetMax().y); y++)
        {
            for (int z = (int) floor(sweep.GetMin().z); z <= (int) floor(sweep.GetMax().z); z++)
            {
                BlockPos pos(x, y, z);
                if (Block::ById(m_level->GetBlockId(pos)) != scaffolding)
                {
                    continue;
                }

                if (!IsSneaking() && min.y >= y + 1 - 1e-5)
                {
                    m_collisionBoxes.push_back(AABB(glm::dvec3(x, y + 14.0 / 16.0, z), glm::dvec3(x + 1, y + 1, z + 1)));
                }
                else if (scaffolding->IsBottom(m_level->GetBlockData(pos)) && min.y >= y + 2.0 / 16.0 - 1e-5)
                {
                    m_collisionBoxes.push_back(AABB(glm::dvec3(x, y, z), glm::dvec3(x + 1, y + 2.0 / 16.0, z + 1)));
                }
            }
        }
    }

    const std::vector<Entity *> &solidEntities = m_level->GetSolidEntities();
    for (size_t i = 0; i < solidEntities.size(); i++)
    {
        Entity *entity = solidEntities[i];
        if (entity != this && entity != m_vehicle && entity->m_vehicle != this && entity->GetAABB().Intersects(sweep))
        {
            m_collisionBoxes.push_back(entity->GetAABB());
        }
    }

    double clippedY = ClipAxis(m_aabb, m_collisionBoxes, 1, dy);
    m_position.y += clippedY;
    UpdateAABB();
    if (clippedY != dy)
    {
        m_onGround          = m_onGround || dy < 0.0;
        m_verticalCollision = true;
        m_velocity.y        = 0.0;
    }

    double clippedX = ClipAxis(m_aabb, m_collisionBoxes, 0, dx);
    m_position.x += clippedX;
    UpdateAABB();
    if (clippedX != dx)
    {
        m_horizontalCollision = true;
        m_velocity.x          = 0.0;
    }

    double clippedZ = ClipAxis(m_aabb, m_collisionBoxes, 2, dz);
    m_position.z += clippedZ;
    UpdateAABB();
    if (clippedZ != dz)
    {
        m_horizontalCollision = true;
        m_velocity.z          = 0.0;
    }
}

void Entity::Move(double dx, double dy, double dz)
{
    bool wasOnGround = m_onGround;

    m_onGround            = false;
    m_horizontalCollision = false;
    m_verticalCollision   = false;

    if (m_noCollision || !m_level)
    {
        m_position.x += dx;
        m_position.y += dy;
        m_position.z += dz;
        UpdateAABB();
        return;
    }

    if (dynamic_cast<LivingEntity *>(this) && IsTouchingBlock(Block::ByName("sweet_berry_bush")))
    {
        dx *= 0.8;
        dy *= 0.75;
        dz *= 0.8;
        m_velocity = {0.0, 0.0, 0.0};
    }

    double walkStartX = m_position.x;
    double walkStartZ = m_position.z;

    if (IsSneaking() && wasOnGround)
    {
        // We probe one step down instead of a full block so the block under a chest or slab never counts as ground
        double stepDown = GetStepHeight();
        // Collision can leave us a hair inside a wall we push against, which would count that wall as ground
        AABB probe = m_aabb.Inflated(-1e-7);
        while (dx != 0.0 && !m_level->IntersectsBlock(probe.Translated(glm::dvec3(dx, -stepDown, 0.0))))
        {
            dx = ReduceTowardZero(dx, s_sneakEdgeStep);
        }

        while (dz != 0.0 && !m_level->IntersectsBlock(probe.Translated(glm::dvec3(0.0, -stepDown, dz))))
        {
            dz = ReduceTowardZero(dz, s_sneakEdgeStep);
        }

        while (dx != 0.0 && dz != 0.0 && !m_level->IntersectsBlock(probe.Translated(glm::dvec3(dx, -stepDown, dz))))
        {
            dx = ReduceTowardZero(dx, s_sneakEdgeStep);
            dz = ReduceTowardZero(dz, s_sneakEdgeStep);
        }
    }

    glm::dvec3 startPosition = m_position;
    glm::dvec3 startVelocity = m_velocity;
    MoveCollide(dx, dy, dz);

    double stepHeight = GetStepHeight();
    if (stepHeight > 0.0 && m_horizontalCollision && (wasOnGround || m_onGround))
    {
        glm::dvec3 flatPosition = m_position;
        glm::dvec3 flatVelocity = m_velocity;
        bool flatOnGround       = m_onGround;
        bool flatVertical       = m_verticalCollision;
        double flatDistanceSq   = (flatPosition.x - startPosition.x) * (flatPosition.x - startPosition.x) +
                                  (flatPosition.z - startPosition.z) * (flatPosition.z - startPosition.z);

        m_position = startPosition;
        m_velocity = startVelocity;
        UpdateAABB();
        m_onGround            = false;
        m_horizontalCollision = false;
        m_verticalCollision   = false;

        // We cap the climb by any ceiling over where we are heading, otherwise a full climb bumps into it and the step fails
        glm::dvec3 min = m_aabb.GetMin();
        glm::dvec3 max = m_aabb.GetMax();
        AABB reach(glm::dvec3(min.x + std::min(dx, 0.0), min.y, min.z + std::min(dz, 0.0)),
                   glm::dvec3(max.x + std::max(dx, 0.0), max.y, max.z + std::max(dz, 0.0)));
        m_level->GetCollisionBoxes(AABB(reach.GetMin(), reach.GetMax() + glm::dvec3(0.0, stepHeight, 0.0)), &m_collisionBoxes);
        MoveCollide(0.0, ClipAxis(reach, m_collisionBoxes, 1, stepHeight), 0.0);
        double climbed        = m_position.y - startPosition.y;
        m_horizontalCollision = false;
        MoveCollide(dx, 0.0, dz);
        bool steppedHorizontal = m_horizontalCollision;
        m_onGround             = false;
        MoveCollide(0.0, -climbed, 0.0);

        double steppedDistanceSq = (m_position.x - startPosition.x) * (m_position.x - startPosition.x) +
                                   (m_position.z - startPosition.z) * (m_position.z - startPosition.z);
        if (steppedDistanceSq <= flatDistanceSq)
        {
            m_position            = flatPosition;
            m_velocity            = flatVelocity;
            m_onGround            = flatOnGround;
            m_verticalCollision   = flatVertical;
            m_horizontalCollision = true;
            UpdateAABB();
        }
        else
        {
            m_velocity.y          = flatVelocity.y;
            m_horizontalCollision = steppedHorizontal;
        }
    }

    BlockPos below((int) floor(m_position.x), (int) floor(m_position.y - 0.2), (int) floor(m_position.z));
    Block *belowBlock = Block::ById(m_level->GetBlockId(below));
    if (m_verticalCollision)
    {
        belowBlock->OnEntityLanded(this, startVelocity.y);
    }

    if (m_onGround && !IsSneaking() && !m_vehicle)
    {
        belowBlock->OnEntityWalk(this);
    }

    double walkMovedX = m_position.x - walkStartX;
    double walkMovedZ = m_position.z - walkStartZ;
    m_walkDist += (float) (sqrt(walkMovedX * walkMovedX + walkMovedZ * walkMovedZ) * 0.6);
}
