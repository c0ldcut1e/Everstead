#include "Boat.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <utility>

#include "../../utils/math/Mth.h"
#include "../../world/Level.h"
#include "../../world/block/Block.h"
#include "../../world/item/Item.h"
#include "../../world/item/ItemInstance.h"
#include "../player/MultiPlayerLocalPlayer.h"
#include "../player/Player.h"
#include "ItemEntity.h"

Boat::Boat(Level *level) : Entity(level)
{
    m_blocksBuilding     = true;
    m_woodType           = 0;
    m_timeSinceHit       = 0;
    m_hurtDir            = 1;
    m_damage             = 0.0f;
    m_paddleStates[0]    = false;
    m_paddleStates[1]    = false;
    m_paddlePositions[0] = 0.0f;
    m_paddlePositions[1] = 0.0f;
    m_deltaRotation      = 0.0f;
    m_outOfControlTicks  = 0.0f;
    m_waterLevel         = 0.0;
    m_lastYd             = 0.0;
    m_glide              = 0.0f;
    m_status             = eStatus_InAir;
    m_previousStatus     = eStatus_InAir;
    m_boundingBox        = {glm::dvec3(-0.6875, 0.0, -0.6875), glm::dvec3(0.6875, 0.5625, 0.6875)};
}

Boat::~Boat()
{}

eENTITY_TYPE Boat::GetType()
{
    return s_type;
}

void Boat::Tick()
{
    m_totalTickCount++;
    m_previousStatus = m_status;
    m_status         = GetStatus();
    if (m_status != eStatus_UnderWater && m_status != eStatus_UnderFlowingWater)
    {
        m_outOfControlTicks = 0.0f;
    }
    else
    {
        m_outOfControlTicks++;
    }

    if (!m_level->IsClientSide() && m_outOfControlTicks >= 60.0f && !m_passengers.empty())
    {
        while (!m_passengers.empty())
        {
            m_passengers[0]->StopRiding();
        }

        m_level->OnPassengersChanged(this);
    }

    if (m_timeSinceHit > 0)
    {
        m_timeSinceHit--;
    }

    if (m_damage > 0.0f)
    {
        m_damage -= 1.0f;
    }

    Player *controller = m_passengers.empty() ? nullptr : dynamic_cast<Player *>(m_passengers[0]);
    bool simulate;
    if (m_level->IsClientSide())
    {
        simulate = controller && controller == m_level->GetLocalPlayer();
    }
    else
    {
        simulate = !controller;
    }

    if (simulate)
    {
        if (!controller)
        {
            SetPaddleState(false, false);
        }

        UpdateMotion();
        if (controller)
        {
            ControlBoat(controller);
        }

        m_lastYd = m_velocity.y;
        Move(m_velocity.x, m_velocity.y, m_velocity.z);
    }
    else
    {
        if (m_level->IsClientSide())
        {
            if (!m_hasNetworkState)
            {
                ApplyNetworkState(m_position, m_yaw, m_pitch);
            }

            TickRemote();
        }

        m_velocity = glm::dvec3();
    }

    for (int i = 0; i < 2; i++)
    {
        if (m_paddleStates[i])
        {
            m_paddlePositions[i] += 0.3926991f;
        }
        else
        {
            m_paddlePositions[i] = 0.0f;
        }
    }

    for (size_t i = 0; i < m_passengers.size(); i++)
    {
        Entity *passenger = m_passengers[i];
        // Other players send their own rotation, turning them here would fight the network lerp
        if (!dynamic_cast<Player *>(passenger) || passenger == m_level->GetLocalPlayer())
        {
            float yaw      = passenger->GetYaw() + Mth::WrapDegrees(m_yaw - m_oldYaw);
            float relative = Mth::WrapDegrees(yaw - m_yaw);
            passenger->SetRotation(yaw + std::clamp(relative, -105.0f, 105.0f) - relative, passenger->GetPitch());
        }

        PositionRider(passenger);
    }
}

void Boat::ReadSpawnData(int data)
{
    SetWoodType(data);
}

int Boat::GetSpawnData()
{
    return m_woodType;
}

void Boat::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    if (tag->Contains("Type"))
    {
        std::string type = tag->GetString("Type");
        for (int i = 0; i < kWoodTypeCount; i++)
        {
            if (type == kWoodTypes[i])
            {
                m_woodType = i;
            }
        }
    }
}

void Boat::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->PutString("Type", kWoodTypes[m_woodType]);
}

void Boat::HandleEntityEvent(uint8_t event)
{
    if (event == kHurtEvent)
    {
        m_hurtDir      = -m_hurtDir;
        m_timeSinceHit = 10;
        m_damage += 10.0f;
    }
}

bool Boat::IsSolid() const
{
    return true;
}

bool Boat::CanAddPassenger(Entity *passenger) const
{
    (void) passenger;
    return m_passengers.size() < 2;
}

void Boat::PositionRider(Entity *passenger)
{
    float offset = 0.0f;
    if (m_passengers.size() > 1)
    {
        offset = passenger == m_passengers[0] ? 0.2f : -0.6f;
    }

    float yawRad = m_yaw * (float) (std::numbers::pi / 180.0);
    passenger->SetPosition(glm::dvec3(m_position.x + cos(yawRad) * offset, m_position.y - 0.45, m_position.z + sin(yawRad) * offset));
}

bool Boat::IsControlledByRider() const
{
    return true;
}

bool Boat::Hurt(int amount, Player *attacker)
{
    m_hurtDir      = -m_hurtDir;
    m_timeSinceHit = 10;
    m_damage += (float) amount * 10.0f;
    m_level->BroadcastEntityEvent(this, kHurtEvent);

    bool creative = attacker->GetGameType() == eGameType_Creative;
    if (!creative && m_damage <= 40.0f)
    {
        return false;
    }

    if (!creative)
    {
        std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
        itemEntity->Sync(m_position, glm::dvec3(m_level->GetRandom().NextDouble() * 0.2 - 0.1, 0.2, m_level->GetRandom().NextDouble() * 0.2 - 0.1),
                         ItemInstance(Item::ByName(std::string(kWoodTypes[m_woodType]) + "_boat"), 1), 0, 10);
        m_level->AddEntity(std::move(itemEntity));
    }

    if (!m_passengers.empty())
    {
        while (!m_passengers.empty())
        {
            m_passengers[0]->StopRiding();
        }

        m_level->OnPassengersChanged(this);
    }

    m_level->RemoveEntity(this);
    return true;
}

void Boat::SetWoodType(int woodType)
{
    m_woodType = std::clamp(woodType, 0, kWoodTypeCount - 1);
}

int Boat::GetWoodType() const
{
    return m_woodType;
}

void Boat::SetPaddleState(bool left, bool right)
{
    if (m_paddleStates[0] == left && m_paddleStates[1] == right)
    {
        return;
    }

    m_paddleStates[0] = left;
    m_paddleStates[1] = right;
    if (!m_level->IsClientSide())
    {
        m_level->OnBoatDataChanged(this);
    }
}

bool Boat::GetPaddleState(int side) const
{
    return m_paddleStates[side];
}

float Boat::GetRowingTime(int side, float partialTicks) const
{
    if (!m_paddleStates[side])
    {
        return 0.0f;
    }

    return std::lerp(m_paddlePositions[side] - 0.3926991f, m_paddlePositions[side], partialTicks);
}

int Boat::GetTimeSinceHit() const
{
    return m_timeSinceHit;
}

int Boat::GetHurtDir() const
{
    return m_hurtDir;
}

float Boat::GetDamage() const
{
    return m_damage;
}

Boat::eSTATUS Boat::GetStatus()
{
    bool flowing;
    if (GetUnderwaterStatus(&flowing))
    {
        m_waterLevel = m_aabb.GetMax().y;
        return flowing ? eStatus_UnderFlowingWater : eStatus_UnderWater;
    }

    if (CheckInWater())
    {
        return eStatus_InWater;
    }

    float glide = GetGlide();
    if (glide > 0.0f)
    {
        m_glide = glide;
        return eStatus_OnLand;
    }

    return eStatus_InAir;
}

bool Boat::CheckInWater()
{
    const glm::dvec3 &min = m_aabb.GetMin();
    const glm::dvec3 &max = m_aabb.GetMax();
    bool inWater          = false;
    m_waterLevel          = -std::numeric_limits<double>::max();
    for (int x = (int) floor(min.x); x < (int) ceil(max.x); x++)
    {
        for (int y = (int) floor(min.y); y < (int) ceil(min.y + 0.001); y++)
        {
            for (int z = (int) floor(min.z); z < (int) ceil(max.z); z++)
            {
                BlockPos pos(x, y, z);
                Block *block = Block::ById(m_level->GetBlockId(pos));
                if (block->IsLiquid() && !block->IsLava())
                {
                    double level = y + block->GetLiquidHeight((uint8_t) m_level->GetBlockData(pos));
                    m_waterLevel = std::max(level, m_waterLevel);
                    inWater |= min.y < level;
                }
            }
        }
    }

    return inWater;
}

bool Boat::GetUnderwaterStatus(bool *flowing) const
{
    const glm::dvec3 &min = m_aabb.GetMin();
    const glm::dvec3 &max = m_aabb.GetMax();
    double top            = max.y + 0.001;
    bool underwater       = false;
    *flowing              = false;
    for (int x = (int) floor(min.x); x < (int) ceil(max.x); x++)
    {
        for (int y = (int) floor(max.y); y < (int) ceil(top); y++)
        {
            for (int z = (int) floor(min.z); z < (int) ceil(max.z); z++)
            {
                BlockPos pos(x, y, z);
                Block *block  = Block::ById(m_level->GetBlockId(pos));
                uint16_t data = m_level->GetBlockData(pos);
                if (block->IsLiquid() && !block->IsLava() && top < y + block->GetLiquidHeight((uint8_t) data))
                {
                    if (data != 0)
                    {
                        *flowing = true;
                        return true;
                    }

                    underwater = true;
                }
            }
        }
    }

    return underwater;
}

float Boat::GetGlide() const
{
    const glm::dvec3 &min = m_aabb.GetMin();
    const glm::dvec3 &max = m_aabb.GetMax();
    AABB under(glm::dvec3(min.x, min.y - 0.001, min.z), glm::dvec3(max.x, min.y, max.z));
    int minX = (int) floor(min.x) - 1;
    int maxX = (int) ceil(max.x) + 1;
    int minY = (int) floor(min.y - 0.001) - 1;
    int maxY = (int) ceil(min.y) + 1;
    int minZ = (int) floor(min.z) - 1;
    int maxZ = (int) ceil(max.z) + 1;

    float glide = 0.0f;
    int count   = 0;
    for (int x = minX; x < maxX; x++)
    {
        for (int z = minZ; z < maxZ; z++)
        {
            int edges = (x == minX || x == maxX - 1 ? 1 : 0) + (z == minZ || z == maxZ - 1 ? 1 : 0);
            if (edges == 2)
            {
                continue;
            }

            for (int y = minY; y < maxY; y++)
            {
                if (edges > 0 && (y == minY || y == maxY - 1))
                {
                    continue;
                }

                glm::dvec3 cellMin(std::max((double) x, under.GetMin().x), std::max((double) y, under.GetMin().y),
                                   std::max((double) z, under.GetMin().z));
                glm::dvec3 cellMax(std::min(x + 1.0, under.GetMax().x), std::min(y + 1.0, under.GetMax().y), std::min(z + 1.0, under.GetMax().z));
                if (cellMin.x > cellMax.x || cellMin.y > cellMax.y || cellMin.z > cellMax.z)
                {
                    continue;
                }

                if (m_level->IntersectsBlock(AABB(cellMin, cellMax)))
                {
                    glide += Block::ById(m_level->GetBlockId(BlockPos(x, y, z)))->GetSlipperiness();
                    count++;
                }
            }
        }
    }

    if (count == 0)
    {
        return 0.0f;
    }

    return glide / (float) count;
}

double Boat::GetWaterLevelAbove() const
{
    const glm::dvec3 &min = m_aabb.GetMin();
    const glm::dvec3 &max = m_aabb.GetMax();
    int minY              = (int) floor(max.y);
    int maxY              = (int) ceil(max.y - m_lastYd);
    for (int y = minY; y < maxY; y++)
    {
        float height = 0.0f;
        for (int x = (int) floor(min.x); x < (int) ceil(max.x) && height < 1.0f; x++)
        {
            for (int z = (int) floor(min.z); z < (int) ceil(max.z) && height < 1.0f; z++)
            {
                BlockPos pos(x, y, z);
                Block *block = Block::ById(m_level->GetBlockId(pos));
                if (block->IsLiquid() && !block->IsLava())
                {
                    height = std::max(height, block->GetLiquidHeight((uint8_t) m_level->GetBlockData(pos)));
                }
            }
        }

        if (height < 1.0f)
        {
            return y + height;
        }
    }

    return maxY + 1.0;
}

void Boat::UpdateMotion()
{
    double height = m_boundingBox.GetMax().y;
    if (m_previousStatus == eStatus_InAir && m_status != eStatus_InAir && m_status != eStatus_OnLand)
    {
        m_waterLevel = m_aabb.GetMin().y + height;
        SetPosition(glm::dvec3(m_position.x, GetWaterLevelAbove() - height + 0.101, m_position.z));
        m_velocity.y = 0.0;
        m_lastYd     = 0.0;
        m_status     = eStatus_InWater;
        return;
    }

    double gravity  = -0.04;
    double buoyancy = 0.0;
    float momentum  = 0.05f;
    switch (m_status)
    {
        case eStatus_InWater:
            buoyancy = (m_waterLevel - m_aabb.GetMin().y) / height;
            momentum = 0.9f;
            break;
        case eStatus_UnderFlowingWater:
            gravity  = -7.0e-4;
            momentum = 0.9f;
            break;
        case eStatus_UnderWater:
            buoyancy = 0.01;
            momentum = 0.45f;
            break;
        case eStatus_InAir:
            momentum = 0.9f;
            break;
        case eStatus_OnLand:
            momentum = m_glide;
            if (!m_passengers.empty() && dynamic_cast<Player *>(m_passengers[0]))
            {
                m_glide /= 2.0f;
            }
            break;
    }

    m_velocity.x *= momentum;
    m_velocity.z *= momentum;
    m_deltaRotation *= momentum;
    m_velocity.y += gravity;
    if (buoyancy > 0.0)
    {
        m_velocity.y += buoyancy * 0.06153846;
        m_velocity.y *= 0.75;
    }
}

void Boat::ControlBoat(Player *player)
{
    bool forward = player->GetMoveForwardInput() > 0.0f;
    bool back    = player->GetMoveForwardInput() < 0.0f;
    bool left    = player->GetMoveStrafeInput() < 0.0f;
    bool right   = player->GetMoveStrafeInput() > 0.0f;

    float speed = 0.0f;
    if (left)
    {
        m_deltaRotation -= 1.0f;
    }

    if (right)
    {
        m_deltaRotation += 1.0f;
    }

    if (right != left && !forward && !back)
    {
        speed += 0.005f;
    }

    SetRotation(m_yaw + m_deltaRotation, m_pitch);
    if (forward)
    {
        speed += 0.04f;
    }

    if (back)
    {
        speed -= 0.005f;
    }

    float yawRad = m_yaw * (float) (std::numbers::pi / 180.0);
    m_velocity.x += cos(yawRad) * speed;
    m_velocity.z += sin(yawRad) * speed;
    SetPaddleState((right && !left) || forward, (left && !right) || forward);
}
