#include "Minecart.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <vector>

#include "../../tag/CompoundTag.h"
#include "../../tag/ListTag.h"
#include "../../utils/math/Mth.h"
#include "../../world/Explosion.h"
#include "../../world/GameRules.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/Block.h"
#include "../../world/block/DetectorRailBlock.h"
#include "../../world/block/PoweredRailBlock.h"
#include "../../world/block/RailBlock.h"
#include "../../world/blockentity/HopperBlockEntity.h"
#include "../../world/inventory/InventoryMenu.h"
#include "../../world/inventory/ServerContainerMenu.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../../world/save/WorldSave.h"
#include "../player/GameType.h"
#include "../player/Player.h"
#include "../player/ServerPlayer.h"
#include "ItemEntity.h"

static const int kExits[10][2][3] = {
        {{0, 0, -1}, {0, 0, 1}},  {{-1, 0, 0}, {1, 0, 0}}, {{-1, -1, 0}, {1, 0, 0}}, {{-1, 0, 0}, {1, -1, 0}}, {{0, 0, -1}, {0, -1, 1}},
        {{0, -1, -1}, {0, 0, 1}}, {{0, 0, 1}, {1, 0, 0}},  {{0, 0, 1}, {-1, 0, 0}},  {{0, 0, -1}, {-1, 0, 0}}, {{0, 0, -1}, {1, 0, 0}},
};
static constexpr double kRailHeight = 0.0625;

Minecart::Minecart(Level *level) : Entity(level)
{
    m_type         = eMinecartType_Rideable;
    m_timeSinceHit = 0;
    m_hurtDir      = 1;
    m_damage       = 0.0f;
    m_flipped      = false;
    m_onRails      = false;
    m_lit          = false;
    m_fuel         = 0;
    m_fuse         = -1;
    m_pushX        = 0.0;
    m_pushZ        = 0.0;
    m_boundingBox  = {glm::dvec3(-0.49, 0.0, -0.49), glm::dvec3(0.49, 0.7, 0.49)};
    // The client box sits half its height above the server one, so a cart never covers the rail it stands on
    if (level->IsClientSide())
    {
        m_boundingBox = m_boundingBox.Translated(glm::dvec3(0.0, 0.35, 0.0));
    }
}

Minecart::~Minecart()
{}

eENTITY_TYPE Minecart::GetType()
{
    return s_type;
}

void Minecart::Tick()
{
    m_totalTickCount++;
    if (m_timeSinceHit > 0)
    {
        m_timeSinceHit--;
    }

    if (m_damage > 0.0f)
    {
        m_damage -= 1.0f;
    }

    if (m_fuse > 0)
    {
        m_fuse--;
    }

    if (m_fuse > 0 && m_level->IsClientSide())
    {
        m_level->SpawnParticle(eParticleType_Smoke, glm::dvec3(m_position.x, m_position.y + 0.5, m_position.z));
    }

    if (m_level->IsClientSide())
    {
        if (m_lit && m_level->GetRandom().NextInt(0, 3) == 0)
        {
            m_level->SpawnParticle(eParticleType_Smoke, glm::dvec3(m_position.x, m_position.y + 0.8, m_position.z));
        }

        if (!m_hasNetworkState)
        {
            ApplyNetworkState(m_position, m_yaw, m_pitch);
        }

        // We replay the server's per tick positions one tick at a time so fast carts follow the rails instead of cutting corners
        while (m_networkSteps.size() > 3)
        {
            m_networkSteps.pop_front();
        }

        if (!m_networkSteps.empty())
        {
            NetworkStep step = m_networkSteps.front();
            m_networkSteps.pop_front();
            m_targetPosition = step.position;
            m_targetYaw      = step.yaw;
            m_targetPitch    = step.pitch;
            m_lerpSteps      = 1;
        }

        TickRemote();
        for (size_t i = 0; i < m_passengers.size(); i++)
        {
            PositionRider(m_passengers[i]);
        }

        return;
    }

    m_oldPosition    = m_position;
    BlockPos railPos = GetRailPos();
    m_onRails        = RailBlock::IsRail(Block::ById(m_level->GetBlockId(railPos)));
    if (m_totalTickCount == 0 && m_onRails)
    {
        AdjustToRails(railPos);
    }

    m_velocity.y -= 0.04;
    double speedSq = m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z;
    MoveAlongTrack();
    if (m_type == eMinecartType_Tnt && (m_fuse == 0 || (m_horizontalCollision && speedSq >= 0.01)))
    {
        Explode(m_fuse == 0 ? m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z : speedSq);
        return;
    }

    if (m_type == eMinecartType_Hopper)
    {
        BlockPos above((int) floor(m_position.x), (int) floor(m_position.y + 1.5), (int) floor(m_position.z));
        AABB area = GetAABB();
        HopperBlockEntity::SuckInItems(m_level, m_container.get(), above,
                                       AABB(area.GetMin() - glm::dvec3(0.25, 0.0, 0.25), area.GetMax() + glm::dvec3(0.25, 0.0, 0.25)), nullptr,
                                       &m_compoundChest);
    }

    Entity *rider = m_passengers.empty() ? nullptr : m_passengers[0];
    AABB area     = GetAABB();
    AABB pushArea(area.GetMin() - glm::dvec3(0.2, 0.0, 0.2), area.GetMax() + glm::dvec3(0.2, 0.0, 0.2));
    bool fast = m_type == eMinecartType_Rideable && m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z > 0.01;
    const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
    for (size_t i = 0; i < entities.size(); i++)
    {
        Minecart *other = dynamic_cast<Minecart *>(entities[i].get());
        if (other && other != this && other->GetAABB().Intersects(pushArea))
        {
            other->PushEntity(this);
        }
    }

    std::vector<Entity *> pushable;
    m_level->GetPushableEntities(fast ? pushArea : area, this, &pushable);
    for (size_t i = 0; i < pushable.size(); i++)
    {
        Entity *entity = pushable[i];
        if (entity == rider || entity->GetVehicle())
        {
            continue;
        }

        if (!fast)
        {
            PushEntity(entity);
        }
        else if (!dynamic_cast<Player *>(entity) && !rider && entity->StartRiding(this))
        {
            rider = entity;
            m_level->OnPassengersChanged(this);
        }
        else
        {
            entity->Push(this);
        }
    }

    if (m_fuel > 0)
    {
        m_fuel--;
    }

    if (m_fuel <= 0)
    {
        m_pushX = 0.0;
        m_pushZ = 0.0;
    }

    SetLit(m_fuel > 0);
    for (size_t i = 0; i < m_passengers.size(); i++)
    {
        PositionRider(m_passengers[i]);
    }
}

bool Minecart::IsDescending(const glm::dvec3 &direction, int shape)
{
    return (shape == 2 && direction.x < 0.0) || (shape == 3 && direction.x > 0.0) || (shape == 4 && direction.z > 0.0) ||
           (shape == 5 && direction.z < 0.0);
}

BlockPos Minecart::GetRailPos() const
{
    BlockPos pos((int) floor(m_position.x), (int) floor(m_position.y), (int) floor(m_position.z));
    int belowY = (int) floor(m_position.y - kRailHeight - 1e-5);
    if (RailBlock::IsRail(Block::ById(m_level->GetBlockId(BlockPos(pos.x, belowY, pos.z)))))
    {
        pos.y = belowY;
    }

    return pos;
}

double Minecart::GetMaxSpeed() const
{
    double maxSpeed = m_level->GetGameRules().GetInt(eGameRule_MinecartMaxSpeed) * (IsInWater() ? 0.5 : 1.0) / 20.0;
    if (m_type == eMinecartType_Furnace)
    {
        maxSpeed *= IsInWater() ? 0.75 : 0.5;
    }

    return maxSpeed;
}

void Minecart::AdjustToRails(const BlockPos &pos)
{
    int shape             = ((RailBlock *) Block::ById(m_level->GetBlockId(pos)))->GetShape(m_level->GetBlockData(pos));
    const int (*exits)[3] = kExits[shape];
    glm::dvec3 h0(exits[0][0] * 0.5, 0.0, exits[0][2] * 0.5);
    glm::dvec3 h1(exits[1][0] * 0.5, 0.0, exits[1][2] * 0.5);
    if ((glm::length(m_velocity) > 1e-5 && glm::dot(m_velocity, h0) < glm::dot(m_velocity, h1)) || IsDescending(h1, shape))
    {
        std::swap(h0, h1);
    }

    glm::dvec3 center(pos.x + 0.5, pos.y, pos.z + 0.5);
    glm::dvec3 newPos(exits[0][2] != exits[1][2] ? center.x : m_position.x, pos.y, exits[0][0] != exits[1][0] ? center.z : m_position.z);
    if (exits[0][0] != exits[1][0] && exits[0][2] != exits[1][2])
    {
        glm::dvec3 segment = h1 - h0;
        glm::dvec3 offset(m_position.x - center.x - h0.x, 0.0, m_position.z - center.z - h0.z);
        newPos = center + h0 + segment * std::clamp(glm::dot(offset, segment) / glm::dot(segment, segment), 0.0, 1.0);
    }

    if (exits[0][1] != exits[1][1])
    {
        int low = exits[0][1] < exits[1][1] ? 0 : 1;
        newPos.y += glm::length(glm::dvec2(newPos.x - center.x - exits[low][0] * 0.5, newPos.z - center.z - exits[low][2] * 0.5));
    }

    newPos.y += kRailHeight;
    Move(newPos.x - m_position.x, newPos.y - m_position.y, newPos.z - m_position.z);
    SetRailYaw((float) (atan2(h1.z - h0.z, h1.x - h0.x) * 180.0 / std::numbers::pi));
}

void Minecart::SetRailYaw(float yaw)
{
    if (m_flipped)
    {
        yaw += 180.0f;
    }

    if (fabsf(Mth::WrapDegrees(yaw - m_yaw)) >= 175.0f)
    {
        m_flipped = !m_flipped;
        yaw -= 180.0f;
    }

    SetRotation(Mth::WrapDegrees(yaw), 0.0f);
}

void Minecart::MoveAlongTrack()
{
    double movementLeft      = 0.0;
    bool firstIteration      = true;
    bool hasGainedSlopeSpeed = false;
    bool hasHalted           = false;
    bool hasBoosted          = false;
    while (firstIteration || movementLeft > 1e-5)
    {
        glm::dvec3 delta = m_velocity;
        glm::dvec3 start = m_position;
        BlockPos pos     = GetRailPos();
        Block *block     = Block::ById(m_level->GetBlockId(pos));
        bool onRails     = RailBlock::IsRail(block);
        if (onRails != m_onRails)
        {
            m_onRails = onRails;
            if (onRails)
            {
                AdjustToRails(pos);
            }
        }

        if (onRails)
        {
            RailBlock *rail  = (RailBlock *) block;
            uint16_t data    = m_level->GetBlockData(pos);
            int shape        = rail->GetShape(data);
            bool poweredRail = dynamic_cast<PoweredRailBlock *>(rail) != nullptr;
            glm::dvec3 speed(delta.x, 0.0, delta.z);
            if (!hasGainedSlopeSpeed && shape >= 2 && shape <= 5)
            {
                double slope = std::max(0.0078125, glm::length(speed) * 0.02);
                if (IsInWater())
                {
                    slope *= 0.2;
                }

                if (shape == 2)
                {
                    speed.x -= slope;
                }
                else if (shape == 3)
                {
                    speed.x += slope;
                }
                else if (shape == 4)
                {
                    speed.z += slope;
                }
                else
                {
                    speed.z -= slope;
                }

                hasGainedSlopeSpeed = true;
            }

            Player *player = m_passengers.empty() ? nullptr : dynamic_cast<Player *>(m_passengers[0]);
            if (firstIteration && player && glm::dot(speed, speed) < 0.01)
            {
                double yawRad  = player->GetYaw() * (std::numbers::pi / 180.0);
                double forward = player->GetMoveForwardInput();
                double strafe  = player->GetMoveStrafeInput();
                glm::dvec3 intent(forward * cos(yawRad) - strafe * sin(yawRad), 0.0, forward * sin(yawRad) + strafe * cos(yawRad));
                if (glm::dot(intent, intent) > 0.0)
                {
                    speed += glm::normalize(intent) * 0.001;
                    hasHalted = true;
                }
            }

            if (!hasHalted && poweredRail && !rail->IsPowered(data) && glm::dot(speed, speed) > 0.0)
            {
                speed     = glm::length(speed) < 0.03 ? glm::dvec3(0.0) : speed * 0.5;
                hasHalted = true;
            }

            if (firstIteration)
            {
                if (m_type == eMinecartType_Furnace)
                {
                    glm::dvec3 push(m_pushX, 0.0, m_pushZ);
                    if (glm::dot(push, push) > 1e-7)
                    {
                        if (glm::dot(push, push) > 1e-4 && glm::dot(speed, speed) > 0.001)
                        {
                            // Projecting onto the velocity can come out near zero, and a zero vector normalizes to NaN in glm
                            glm::dvec3 projected = speed * (glm::dot(push, speed) / glm::dot(speed, speed));
                            push                 = glm::length(projected) < 1e-5 ? glm::dvec3(0.0) : glm::normalize(projected) * glm::length(push);
                            m_pushX              = push.x;
                            m_pushZ              = push.z;
                        }

                        speed = speed * 0.8 + push;
                        if (IsInWater())
                        {
                            speed *= 0.1;
                        }
                    }
                    else
                    {
                        speed *= 0.98;
                    }
                }

                speed *= m_passengers.empty() ? 0.96 : 0.997;
                if (IsInWater())
                {
                    speed *= 0.95;
                }

                double maxSpeed = GetMaxSpeed();
                if (glm::length(speed) > maxSpeed)
                {
                    speed = glm::normalize(speed) * maxSpeed;
                }
            }

            if (!hasBoosted && poweredRail && rail->IsPowered(data))
            {
                double length = glm::length(speed);
                glm::dvec3 direction(0.0);
                if (length > 0.01)
                {
                    direction = speed / length;
                    length += 0.06;
                }
                else
                {
                    if (shape == 1 && Block::ById(m_level->GetBlockId(BlockPos(pos.x - 1, pos.y, pos.z)))->IsSolid())
                    {
                        direction.x = 1.0;
                    }
                    else if (shape == 1 && Block::ById(m_level->GetBlockId(BlockPos(pos.x + 1, pos.y, pos.z)))->IsSolid())
                    {
                        direction.x = -1.0;
                    }
                    else if (shape == 0 && Block::ById(m_level->GetBlockId(BlockPos(pos.x, pos.y, pos.z - 1)))->IsSolid())
                    {
                        direction.z = 1.0;
                    }
                    else if (shape == 0 && Block::ById(m_level->GetBlockId(BlockPos(pos.x, pos.y, pos.z + 1)))->IsSolid())
                    {
                        direction.z = -1.0;
                    }

                    length += 0.2;
                }

                if (glm::dot(direction, direction) > 0.0)
                {
                    speed      = direction * length;
                    hasBoosted = true;
                }
            }

            movementLeft = firstIteration ? glm::length(speed) : movementLeft + glm::length(speed) - glm::length(glm::dvec2(delta.x, delta.z));
            m_velocity   = speed;
            movementLeft = StepAlongTrack(pos, shape, movementLeft);
            DetectorRailBlock *detector = dynamic_cast<DetectorRailBlock *>(block);
            if (detector)
            {
                detector->OnMinecartInside(m_level, pos);
            }
        }
        else
        {
            double maxSpeed = GetMaxSpeed();
            m_velocity.x    = std::clamp(m_velocity.x, -maxSpeed, maxSpeed);
            m_velocity.z    = std::clamp(m_velocity.z, -maxSpeed, maxSpeed);
            if (m_onGround)
            {
                m_velocity *= 0.5;
            }

            Move(m_velocity.x, m_velocity.y, m_velocity.z);
            if (!m_onGround)
            {
                m_velocity *= 0.95;
            }

            movementLeft = 0.0;
        }

        glm::dvec3 moved = m_position - start;
        if (moved.x * moved.x + moved.z * moved.z > 1e-5)
        {
            SetRailYaw((float) (atan2(-moved.z, -moved.x) * 180.0 / std::numbers::pi));
        }

        firstIteration = false;
    }
}

double Minecart::StepAlongTrack(const BlockPos &pos, int shape, double movementLeft)
{
    glm::dvec3 h(m_velocity.x, 0.0, m_velocity.z);
    if (movementLeft < 1e-5)
    {
        return 0.0;
    }

    if (glm::length(h) < 1e-5)
    {
        m_velocity = glm::dvec3(0.0);
        return 0.0;
    }

    const int (*exits)[3] = kExits[shape];
    bool slope            = exits[0][1] != exits[1][1];
    bool descending       = IsDescending(h, shape);
    glm::dvec3 e0(exits[0][0] * 0.5, 0.0, exits[0][2] * 0.5);
    glm::dvec3 e1(exits[1][0] * 0.5, 0.0, exits[1][2] * 0.5);
    glm::dvec3 exit   = glm::dot(h, e0) < glm::dot(h, e1) ? e1 : e0;
    glm::dvec3 target = glm::dvec3(pos.x + 0.5, pos.y + kRailHeight, pos.z + 0.5) + exit + glm::normalize(exit) * 1e-5;
    if (slope && !descending)
    {
        target.y += 1.0;
    }

    glm::dvec3 start     = m_position;
    glm::dvec3 direction = glm::normalize(target - start);
    h                    = direction * (glm::length(h) / glm::length(glm::dvec2(direction.x, direction.z)));
    glm::dvec3 next      = start + direction * movementLeft * (slope ? std::numbers::sqrt2 : 1.0);
    if (glm::distance(start, target) <= glm::distance(start, next))
    {
        movementLeft = glm::length(glm::dvec2(target.x - next.x, target.z - next.z));
        next         = target;
    }
    else
    {
        movementLeft = 0.0;
    }

    Move(next.x - start.x, next.y - start.y, next.z - start.z);
    if (slope)
    {
        BlockPos nextPos((int) floor(next.x), (int) floor(next.y), (int) floor(next.z));
        Block *nextBlock = Block::ById(m_level->GetBlockId(nextPos));
        if (RailBlock::IsRail(nextBlock))
        {
            int nextShape = ((RailBlock *) nextBlock)->GetShape(m_level->GetBlockData(nextPos));
            // We stop at the bottom of a V so a slow cart doesn't rock back and forth between the two slopes forever
            if (glm::dot(m_velocity, m_velocity) < 0.005 && nextShape >= 2 && nextShape <= 5 && IsDescending(m_velocity, shape) &&
                !IsDescending(m_velocity, nextShape))
            {
                m_velocity = glm::dvec3(0.0);
                return 0.0;
            }
        }

        double distance = glm::length(glm::dvec2(target.x - m_position.x, target.z - m_position.z));
        double y        = target.y + (descending ? distance : -distance);
        if (m_position.y < y)
        {
            Move(0.0, y - m_position.y, 0.0);
        }
    }

    if (glm::length(m_position - start) < 1e-5 && glm::length(next - start) > 1e-5)
    {
        m_velocity = glm::dvec3(0.0);
        return 0.0;
    }

    m_velocity = h;
    return movementLeft;
}

void Minecart::PushEntity(Entity *entity)
{
    glm::dvec3 otherPos = entity->GetPosition();
    double xa           = otherPos.x - m_position.x;
    double za           = otherPos.z - m_position.z;
    double dd           = xa * xa + za * za;
    if (dd < 1e-4)
    {
        return;
    }

    dd = sqrt(dd);
    xa /= dd;
    za /= dd;
    double pow = std::min(1.0 / dd, 1.0);
    xa *= pow * 0.05;
    za *= pow * 0.05;

    Minecart *other = dynamic_cast<Minecart *>(entity);
    if (!other)
    {
        m_velocity.x -= xa;
        m_velocity.z -= za;
        glm::dvec3 velocity = entity->GetVelocity();
        entity->SetVelocity(glm::dvec3(velocity.x + xa / 4.0, velocity.y, velocity.z + za / 4.0));
        return;
    }

    double xo    = otherPos.x - m_position.x;
    double zo    = otherPos.z - m_position.z;
    double lenO  = sqrt(xo * xo + zo * zo);
    float yawRad = m_yaw * (float) (std::numbers::pi / 180.0);
    if (fabs((xo * cos(yawRad) + zo * sin(yawRad)) / lenO) < 0.8)
    {
        return;
    }

    glm::dvec3 otherVelocity = other->GetVelocity();
    double xdd               = otherVelocity.x + m_velocity.x;
    double zdd               = otherVelocity.z + m_velocity.z;
    if (other->m_type == eMinecartType_Furnace && m_type != eMinecartType_Furnace)
    {
        m_velocity.x *= 0.2;
        m_velocity.z *= 0.2;
        m_velocity.x += otherVelocity.x * 0.8 - xa;
        m_velocity.z += otherVelocity.z * 0.8 - za;
        otherVelocity.x *= 0.95;
        otherVelocity.z *= 0.95;
    }
    else if (other->m_type != eMinecartType_Furnace && m_type == eMinecartType_Furnace)
    {
        otherVelocity.x *= 0.2;
        otherVelocity.z *= 0.2;
        otherVelocity.x += m_velocity.x * 0.8 + xa;
        otherVelocity.z += m_velocity.z * 0.8 + za;
        m_velocity.x *= 0.95;
        m_velocity.z *= 0.95;
    }
    else
    {
        // Keeping 0.2 of our own velocity on top of the full mean would add speed every tick, so the mean is scaled to conserve momentum
        xdd *= 0.4;
        zdd *= 0.4;
        m_velocity.x *= 0.2;
        m_velocity.z *= 0.2;
        m_velocity.x += xdd - xa;
        m_velocity.z += zdd - za;
        otherVelocity.x *= 0.2;
        otherVelocity.z *= 0.2;
        otherVelocity.x += xdd + xa;
        otherVelocity.z += zdd + za;
    }

    other->SetVelocity(otherVelocity);
}

void Minecart::ReadSpawnData(int data)
{
    SetMinecartType(data & 7);
    m_lit = (data & 8) != 0;
}

int Minecart::GetSpawnData()
{
    return m_type | (m_lit ? 8 : 0);
}

void Minecart::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    SetMinecartType(tag->GetInt("Type"));
    if (m_type == eMinecartType_Furnace)
    {
        m_pushX = tag->GetDouble("PushX");
        m_pushZ = tag->GetDouble("PushZ");
        m_fuel  = tag->GetShort("Fuel");
    }
    else if (m_type == eMinecartType_Tnt)
    {
        m_fuse = tag->GetInt("TNTFuse");
    }
    else if (m_container)
    {
        ListTag *items = tag->GetList("Items");
        for (size_t i = 0; items && i < items->Size(); i++)
        {
            const CompoundTag *itemTag = (const CompoundTag *) items->Get(i);
            int slot                   = itemTag->GetByte("Slot");
            if (slot >= 0 && slot < m_container->GetSize())
            {
                m_container->SetItem(slot, WorldSave::ReadItemTag(itemTag));
            }
        }
    }
}

void Minecart::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->PutInt("Type", m_type);
    if (m_type == eMinecartType_Furnace)
    {
        tag->PutDouble("PushX", m_pushX);
        tag->PutDouble("PushZ", m_pushZ);
        tag->PutShort("Fuel", (int16_t) m_fuel);
    }
    else if (m_type == eMinecartType_Tnt)
    {
        tag->PutInt("TNTFuse", m_fuse);
    }
    else if (m_container)
    {
        std::unique_ptr<ListTag> items(new ListTag());
        for (int i = 0; i < m_container->GetSize(); i++)
        {
            const ItemInstance *item = ((const Container *) m_container.get())->GetItem(i);
            if (item->IsEmpty())
            {
                continue;
            }

            std::unique_ptr<CompoundTag> itemTag = WorldSave::CreateItemTag(*item);
            itemTag->PutByte("Slot", (int8_t) i);
            items->Add(std::move(itemTag));
        }

        tag->Put("Items", std::move(items));
    }
}

void Minecart::HandleEntityEvent(uint8_t event)
{
    if (event == kHurtEvent)
    {
        m_hurtDir      = -m_hurtDir;
        m_timeSinceHit = 10;
        m_damage += 10.0f;
    }
    else if (event == kLitEvent || event == kUnlitEvent)
    {
        m_lit = event == kLitEvent;
    }
    else if (event == kPrimeEvent)
    {
        m_fuse = 80;
    }
}

int Minecart::GetUpdateInterval() const
{
    return 1;
}

void Minecart::ApplyNetworkState(const glm::dvec3 &position, float yaw, float pitch)
{
    if (!m_hasNetworkState)
    {
        Entity::ApplyNetworkState(position, yaw, pitch);
        return;
    }

    m_networkSteps.push_back({position, yaw, pitch});
}

bool Minecart::CanAddPassenger(Entity *passenger) const
{
    (void) passenger;
    return m_type == eMinecartType_Rideable && m_passengers.empty();
}

void Minecart::PositionRider(Entity *passenger)
{
    passenger->SetPosition(glm::dvec3(m_position.x, m_position.y - 0.35, m_position.z));
}

glm::dvec3 Minecart::GetDismountPosition(Entity *passenger) const
{
    float yawRad = m_yaw * (float) (std::numbers::pi / 180.0);
    int forwardX = 0;
    int forwardZ = 0;
    if (fabsf(cosf(yawRad)) >= fabsf(sinf(yawRad)))
    {
        forwardX = cosf(yawRad) >= 0.0f ? 1 : -1;
    }
    else
    {
        forwardZ = sinf(yawRad) >= 0.0f ? 1 : -1;
    }

    const int kOffsets[8][2] = {{0, 1}, {0, -1}, {-1, 1}, {-1, -1}, {1, 1}, {1, -1}, {-1, 0}, {1, 0}};
    glm::dvec3 size          = passenger->GetAABB().GetMax() - passenger->GetAABB().GetMin();
    int railY                = (int) floor(GetAABB().GetMin().y);
    glm::dvec3 result        = Entity::GetDismountPosition(passenger);
    bool foundStep           = false;
    std::vector<AABB> boxes;
    for (int i = 0; i < 8; i++)
    {
        int x = (int) floor(m_position.x) + forwardX * kOffsets[i][0] - forwardZ * kOffsets[i][1];
        int z = (int) floor(m_position.z) + forwardZ * kOffsets[i][0] + forwardX * kOffsets[i][1];
        for (int y = railY; y <= railY + 1; y++)
        {
            glm::dvec3 feet(x + 0.5, (double) y, z + 0.5);
            boxes.clear();
            m_level->GetCollisionBoxes(
                    AABB(feet - glm::dvec3(size.x / 2.0, 0.0, size.z / 2.0), feet + glm::dvec3(size.x / 2.0, size.y, size.z / 2.0)), &boxes);
            if (!boxes.empty() || !Block::ById(m_level->GetBlockId(BlockPos(x, y - 1, z)))->IsSolid())
            {
                continue;
            }

            if (y == railY)
            {
                return feet;
            }

            if (!foundStep)
            {
                result    = feet;
                foundStep = true;
            }
        }
    }

    return result;
}

bool Minecart::Hurt(int amount, Player *attacker)
{
    m_hurtDir      = -m_hurtDir;
    m_timeSinceHit = 10;
    m_damage += (float) amount * 10.0f;
    if (!m_passengers.empty() && m_passengers[0] == attacker)
    {
        m_damage += 1.0f;
    }

    m_level->BroadcastEntityEvent(this, kHurtEvent);

    bool creative = attacker->GetGameType() == eGameType_Creative;
    if (!creative && m_damage > 40.0f && m_type == eMinecartType_Tnt && m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z >= 0.01)
    {
        PrimeFuse(m_level->GetRandom().NextInt(0, 19) + m_level->GetRandom().NextInt(0, 19));
        return false;
    }

    if (!creative && m_damage <= 40.0f)
    {
        return false;
    }

    if (!m_passengers.empty())
    {
        while (!m_passengers.empty())
        {
            m_passengers[0]->StopRiding();
        }

        m_level->OnPassengersChanged(this);
    }

    if (!creative)
    {
        const char *kBlocks[] = {"", "chest", "furnace", "tnt", "hopper"};
        const char *drops[2]  = {"minecart", kBlocks[m_type]};
        for (int i = 0; i < (m_type == eMinecartType_Rideable ? 1 : 2); i++)
        {
            std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
            itemEntity->Sync(m_position,
                             glm::dvec3(m_level->GetRandom().NextDouble() * 0.2 - 0.1, 0.2, m_level->GetRandom().NextDouble() * 0.2 - 0.1),
                             ItemInstance(Item::ByName(drops[i]), 1), 0, 10);
            m_level->AddEntity(std::move(itemEntity));
        }
    }

    if (m_container)
    {
        ((ServerLevel *) m_level)
                ->DropContainerItems(BlockPos((int) floor(m_position.x), (int) floor(m_position.y), (int) floor(m_position.z)), m_container.get());
    }

    m_level->RemoveEntity(this);
    return true;
}

bool Minecart::Interact(ServerPlayer *player)
{
    if (m_container)
    {
        player->GetContainerMenu()->OpenMinecart(this, m_type == eMinecartType_Hopper ? InventoryMenu::s_hopperMenuId : InventoryMenu::s_chestMenuId);
        return false;
    }

    if (m_type == eMinecartType_Furnace)
    {
        ItemInstance *held = player->GetInventory()->GetSelectedItem();
        bool consumed      = false;
        if (!held->IsEmpty() && held->GetItem() == Item::ByName("coal"))
        {
            if (!GameType::HasInfiniteItems(player->GetGameType()))
            {
                held->ConsumeOne();
                consumed = true;
            }

            m_fuel += 3600;
        }

        m_pushX = m_position.x - player->GetPosition().x;
        m_pushZ = m_position.z - player->GetPosition().z;
        return consumed;
    }

    if (!m_passengers.empty())
    {
        if (m_passengers[0] == player)
        {
            player->DismountVehicle();
        }

        return false;
    }

    if (!player->IsSneaking() && !player->GetVehicle() && player->StartRiding(this))
    {
        m_level->OnPassengersChanged(this);
    }

    return false;
}

void Minecart::SetMinecartType(int type)
{
    m_type = std::clamp(type, 0, (int) eMinecartType_Hopper);
    if (m_type == eMinecartType_Chest)
    {
        m_container.reset(new Container(3, 9));
    }
    else if (m_type == eMinecartType_Hopper)
    {
        m_container.reset(new Container(1, HopperBlockEntity::s_slotCount));
    }
    else
    {
        m_container.reset();
    }
}

int Minecart::GetMinecartType() const
{
    return m_type;
}

bool Minecart::IsLit() const
{
    return m_lit;
}

int Minecart::GetFuse() const
{
    return m_fuse;
}

Container *Minecart::GetContainer() const
{
    return m_container.get();
}

void Minecart::PrimeFuse(int fuse)
{
    if (m_fuse >= 0)
    {
        return;
    }

    m_fuse = fuse;
    m_level->BroadcastEntityEvent(this, kPrimeEvent);
    m_level->PlaySound("random.fuse", m_position, 1.0f, 1.0f);
}

void Minecart::Explode(double speedSq)
{
    Explosion(m_level, m_position, (float) (4.0 + m_level->GetRandom().NextDouble() * 1.5 * std::min(sqrt(speedSq), 5.0))).Explode();
    m_level->RemoveEntity(this);
}

void Minecart::SetLit(bool lit)
{
    if (m_lit == lit)
    {
        return;
    }

    m_lit = lit;
    m_level->BroadcastEntityEvent(this, lit ? kLitEvent : kUnlitEvent);
}

int Minecart::GetTimeSinceHit() const
{
    return m_timeSinceHit;
}

int Minecart::GetHurtDir() const
{
    return m_hurtDir;
}

float Minecart::GetDamage() const
{
    return m_damage;
}

bool Minecart::GetPos(glm::dvec3 position, glm::dvec3 *out) const
{
    BlockPos pos((int) floor(position.x), (int) floor(position.y), (int) floor(position.z));
    if (RailBlock::IsRail(Block::ById(m_level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)))))
    {
        pos.y--;
    }

    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (!RailBlock::IsRail(block))
    {
        return false;
    }

    const int (*exits)[3] = kExits[((RailBlock *) block)->GetShape(m_level->GetBlockData(pos))];
    double x0             = pos.x + 0.5 + exits[0][0] * 0.5;
    double y0             = pos.y + 0.0625 + exits[0][1] * 0.5;
    double z0             = pos.z + 0.5 + exits[0][2] * 0.5;
    double xD             = pos.x + 0.5 + exits[1][0] * 0.5 - x0;
    double yD             = (pos.y + 0.0625 + exits[1][1] * 0.5 - y0) * 2.0;
    double zD             = pos.z + 0.5 + exits[1][2] * 0.5 - z0;
    double progress       = ((position.x - x0) * xD + (position.z - z0) * zD) * 2.0;
    if (xD == 0.0)
    {
        progress = position.z - pos.z;
    }
    else if (zD == 0.0)
    {
        progress = position.x - pos.x;
    }

    double y = y0 + yD * progress;
    if (yD < 0.0)
    {
        y += 1.0;
    }
    else if (yD > 0.0)
    {
        y += 0.5;
    }

    *out = glm::dvec3(x0 + xD * progress, y, z0 + zD * progress);
    return true;
}

bool Minecart::GetPosOffs(glm::dvec3 position, double offset, glm::dvec3 *out) const
{
    BlockPos pos((int) floor(position.x), (int) floor(position.y), (int) floor(position.z));
    if (RailBlock::IsRail(Block::ById(m_level->GetBlockId(BlockPos(pos.x, pos.y - 1, pos.z)))))
    {
        pos.y--;
    }

    Block *block = Block::ById(m_level->GetBlockId(pos));
    if (!RailBlock::IsRail(block))
    {
        return false;
    }

    int shape             = ((RailBlock *) block)->GetShape(m_level->GetBlockData(pos));
    double y              = shape >= 2 && shape <= 5 ? pos.y + 1.0 : pos.y;
    const int (*exits)[3] = kExits[shape];
    double xD             = exits[1][0] - exits[0][0];
    double zD             = exits[1][2] - exits[0][2];
    double length         = sqrt(xD * xD + zD * zD);
    double x              = position.x + xD / length * offset;
    double z              = position.z + zD / length * offset;
    for (int i = 0; i < 2; i++)
    {
        if (exits[i][1] != 0 && (int) floor(x) - pos.x == exits[i][0] && (int) floor(z) - pos.z == exits[i][2])
        {
            y += exits[i][1];
            break;
        }
    }

    return GetPos(glm::dvec3(x, y, z), out);
}
