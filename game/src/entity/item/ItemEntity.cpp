#include "ItemEntity.h"

#include <cmath>
#include <numbers>

#include "../../world/Level.h"
#include "../../world/block/Block.h"
#include "../../world/save/WorldSave.h"

static const double s_gravity   = 0.04;
static const double s_drag      = 0.98;
static const double s_bounce    = -0.5;
static const double s_halfWidth = 0.125;
static const double s_height    = 0.25;

ItemEntity::ItemEntity(Level *level) : Entity(level)
{
    m_liquidInsetY   = 0.001;
    Random &random   = level->GetRandom();
    m_item           = ItemInstance();
    m_age            = 0;
    m_pickupDelay    = 0;
    m_bobOffset      = random.NextFloat() * (float) std::numbers::pi * 2.0f;
    m_landed         = false;
    m_collecting     = false;
    m_collectorIndex = 0;
    m_collectAge     = 0;
    m_noGravity      = true;
    m_boundingBox    = {glm::dvec3(-s_halfWidth, 0.0, -s_halfWidth), glm::dvec3(s_halfWidth, s_height, s_halfWidth)};
    m_yaw            = random.NextFloat() * 360.0f;
}

ItemEntity::~ItemEntity()
{}

eENTITY_TYPE ItemEntity::GetType()
{
    return s_type;
}

bool ItemEntity::IsPickable() const
{
    return false;
}

void ItemEntity::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    CompoundTag *itemTag = tag->GetCompound("Item");
    if (itemTag)
    {
        m_item = WorldSave::ReadItemTag(itemTag);
    }

    if (tag->Contains("Age"))
    {
        m_age = tag->GetShort("Age");
    }

    if (tag->Contains("PickupDelay"))
    {
        m_pickupDelay = tag->GetShort("PickupDelay");
    }
}

void ItemEntity::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    tag->Put("Item", WorldSave::CreateItemTag(m_item));
    tag->PutShort("Age", (int16_t) m_age);
    tag->PutShort("PickupDelay", (int16_t) m_pickupDelay);
}

void ItemEntity::Tick()
{
    m_totalTickCount++;
    if (!m_level->IsClientSide() && m_item.IsEmpty())
    {
        m_level->RemoveEntity(this);
        return;
    }

    if (m_collecting)
    {
        m_collectAge++;
        if (m_collectAge >= s_collectTicks)
        {
            m_level->RemoveEntity(this);
        }

        return;
    }

    if (m_pickupDelay > 0)
    {
        m_pickupDelay--;
    }

    UpdateAABB();
    bool wasOnGround = m_onGround;
    if (!m_onGround || m_velocity.x * m_velocity.x + m_velocity.z * m_velocity.z > 1.0e-5 || (m_totalTickCount + GetId()) % 4 == 0)
    {
        if (IsInWater())
        {
            ApplyWaterFlow();
        }

        if (GetLiquidDepth(false) > 0.1)
        {
            FloatInLiquid(0.99);
        }
        else if (GetLiquidDepth(true) > 0.1)
        {
            FloatInLiquid(0.95);
        }
        else
        {
            m_velocity.y -= s_gravity;
        }

        m_noCollision = PushOutOfBlocks(m_position.x, (m_aabb.GetMin().y + m_aabb.GetMax().y) / 2.0, m_position.z);
        Move(m_velocity.x, m_velocity.y, m_velocity.z);

        double friction = s_drag;
        if (m_onGround)
        {
            friction = GetGroundSlipperiness() * s_drag;
        }

        m_velocity.x *= friction;
        m_velocity.y *= s_drag;
        m_velocity.z *= friction;
        if (m_onGround)
        {
            m_velocity.y *= s_bounce;
        }
    }

    if (m_onGround && !wasOnGround)
    {
        m_landed = true;
    }

    m_age++;
}

int ItemEntity::GetUpdateInterval() const
{
    return 20;
}

bool ItemEntity::SendsVelocity() const
{
    return true;
}

bool ItemEntity::IsNormalCube(int x, int y, int z) const
{
    Block *block = Block::ById(m_level->GetBlockId(BlockPos(x, y, z)));
    return block->IsSolid() && block->IsCollidable() && !block->IsTransparent() && block->GetRenderShape() == Block::eRenderShape_CUBE;
}

bool ItemEntity::PushOutOfBlocks(double x, double y, double z)
{
    int blockX   = (int) floor(x);
    int blockY   = (int) floor(y);
    int blockZ   = (int) floor(z);
    double fracX = x - blockX;
    double fracY = y - blockY;
    double fracZ = z - blockZ;

    if (!m_level->IntersectsBlock(m_aabb) && !IsNormalCube(blockX, blockY, blockZ))
    {
        return false;
    }

    int side       = 3;
    double nearest = 9999.0;
    if (!IsNormalCube(blockX - 1, blockY, blockZ) && fracX < nearest)
    {
        nearest = fracX;
        side    = 0;
    }

    if (!IsNormalCube(blockX + 1, blockY, blockZ) && 1.0 - fracX < nearest)
    {
        nearest = 1.0 - fracX;
        side    = 1;
    }

    if (!IsNormalCube(blockX, blockY + 1, blockZ) && 1.0 - fracY < nearest)
    {
        nearest = 1.0 - fracY;
        side    = 3;
    }

    if (!IsNormalCube(blockX, blockY, blockZ - 1) && fracZ < nearest)
    {
        nearest = fracZ;
        side    = 4;
    }

    if (!IsNormalCube(blockX, blockY, blockZ + 1) && 1.0 - fracZ < nearest)
    {
        side = 5;
    }

    float speed = m_level->GetRandom().NextFloat() * 0.2f + 0.1f;
    if (side == 0)
    {
        m_velocity.x = -speed;
    }
    else if (side == 1)
    {
        m_velocity.x = speed;
    }
    else if (side == 3)
    {
        m_velocity.y = speed;
    }
    else if (side == 4)
    {
        m_velocity.z = -speed;
    }
    else
    {
        m_velocity.z = speed;
    }

    return true;
}

void ItemEntity::Sync(const glm::dvec3 &position, const glm::dvec3 &velocity, const ItemInstance &item, int age, int pickupDelay)
{
    bool spawned = m_item.IsEmpty();
    SetPosition(position);
    UpdateAABB();
    if (spawned)
    {
        StoreOld();
    }

    m_velocity    = velocity;
    m_item        = item;
    m_age         = age;
    m_pickupDelay = pickupDelay;
}

ItemInstance &ItemEntity::GetItem()
{
    return m_item;
}

const ItemInstance &ItemEntity::GetItem() const
{
    return m_item;
}

void ItemEntity::SetItem(const ItemInstance &item)
{
    m_item = item;
}

int ItemEntity::GetAge() const
{
    return m_age;
}

void ItemEntity::SetAge(int age)
{
    m_age = age;
}

int ItemEntity::GetPickupDelay() const
{
    return m_pickupDelay;
}

void ItemEntity::SetPickupDelay(int pickupDelay)
{
    m_pickupDelay = pickupDelay;
}

float ItemEntity::GetBobOffset() const
{
    return m_bobOffset;
}

bool ItemEntity::TakeLanded()
{
    bool landed = m_landed;
    m_landed    = false;
    return landed;
}

void ItemEntity::StartCollect(const ItemEntity *source, uint8_t playerIndex, int count)
{
    SetPosition(source->GetPosition());
    UpdateAABB();
    StoreOld();
    m_item = source->GetItem();
    m_item.SetCount(count);
    m_age            = source->GetAge();
    m_bobOffset      = source->GetBobOffset();
    m_collecting     = true;
    m_collectorIndex = playerIndex;
    m_collectAge     = 0;
}

bool ItemEntity::IsCollecting() const
{
    return m_collecting;
}

uint8_t ItemEntity::GetCollectorIndex() const
{
    return m_collectorIndex;
}

float ItemEntity::GetCollectProgress(float partialTicks) const
{
    float progress = ((float) m_collectAge + partialTicks) / (float) s_collectTicks;
    return progress > 1.0f ? 1.0f : progress * progress;
}
