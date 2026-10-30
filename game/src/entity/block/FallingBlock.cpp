#include "FallingBlock.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "../../core/EversteadServer.h"
#include "../../network/PlayerList.h"
#include "../../world/Level.h"
#include "../../world/ServerLevel.h"
#include "../../world/block/AnvilBlock.h"
#include "../../world/block/Block.h"
#include "../../world/block/BlockRegistry.h"
#include "../../world/block/GravityBlock.h"
#include "../../world/block/PointedDripstoneBlock.h"
#include "../../world/chunk/Chunk.h"
#include "../../world/item/Item.h"
#include "../../world/particle/ParticleSpawnParams.h"
#include "../item/ItemEntity.h"
#include "../player/ServerPlayer.h"

static const double s_gravity  = 0.04;
static const double s_drag     = 0.98;
static const int s_maxLifetime = 600;

FallingBlock::FallingBlock(Level *level) : Entity(level)
{
    m_blocksBuilding = true;
    m_liquidInsetY   = 0.001;
    m_block          = nullptr;
    m_blockData      = 0;
    m_spawnData      = 0;
    m_fallDistance   = 0.0f;
    m_noGravity      = true;
    m_boundingBox    = {glm::dvec3(-0.49, 0.0, -0.49), glm::dvec3(0.49, 0.98, 0.49)};
}

FallingBlock::~FallingBlock()
{}

eENTITY_TYPE FallingBlock::GetType()
{
    return s_type;
}

void FallingBlock::Tick()
{
    m_totalTickCount++;
    if (!m_block || m_totalTickCount > s_maxLifetime)
    {
        m_level->RemoveEntity(this);
        return;
    }

    UpdateAABB();
    if (IsInWater())
    {
        ApplyWaterFlow();
    }

    m_velocity.y -= s_gravity;
    double oldY = m_position.y;
    Move(m_velocity.x, m_velocity.y, m_velocity.z);
    m_fallDistance += (float) std::max(oldY - m_position.y, 0.0);
    m_velocity.x *= s_drag;
    m_velocity.y *= s_drag;
    m_velocity.z *= s_drag;

    if (m_onGround)
    {
        Land();
    }
}

int FallingBlock::GetUpdateInterval() const
{
    return 20;
}

bool FallingBlock::SendsVelocity() const
{
    return true;
}

void FallingBlock::ReadSpawnData(int data)
{
    m_spawnData = data;
    m_block     = Block::ById((uint32_t) ((data >> 16) & 0xFFFF));
    m_blockData = (uint16_t) (data & 0xFFFF);
    m_random    = Random((uint64_t) (uint32_t) data);
}

int FallingBlock::GetSpawnData()
{
    return m_spawnData;
}

void FallingBlock::Load(const CompoundTag *tag)
{
    Entity::Load(tag);
    CompoundTag *stateTag = tag->GetCompound("BlockState");
    if (stateTag)
    {
        Block *block = Block::ByName(stateTag->GetString("Name"));
        if (block)
        {
            uint16_t data             = 0;
            CompoundTag *propertyTags = stateTag->GetCompound("Properties");
            for (size_t i = 0; propertyTags && i < block->GetProperties().size(); i++)
            {
                const Block::Property &property = block->GetProperties()[i];
                if (propertyTags->Contains(property.name))
                {
                    data = block->WithPropertyValue(data, (int) i, std::clamp(propertyTags->GetInt(property.name), 0, property.valueCount - 1));
                }
            }

            ReadSpawnData((int) ((BlockRegistry::GetInstance()->IdOf(block) << 16) | data));
        }
    }

    if (tag->Contains("Time"))
    {
        m_totalTickCount = tag->GetInt("Time");
    }

    if (tag->Contains("FallDistance"))
    {
        m_fallDistance = tag->GetFloat("FallDistance");
    }
}

void FallingBlock::Save(CompoundTag *tag) const
{
    Entity::Save(tag);
    std::unique_ptr<CompoundTag> stateTag(new CompoundTag());
    stateTag->PutString("Name", m_block->GetResourceLocation().ToString());
    std::unique_ptr<CompoundTag> propertyTags(new CompoundTag());
    for (size_t i = 0; i < m_block->GetProperties().size(); i++)
    {
        propertyTags->PutInt(m_block->GetProperties()[i].name, m_block->GetPropertyValue(m_blockData, (int) i));
    }

    stateTag->Put("Properties", std::move(propertyTags));
    tag->Put("BlockState", std::move(stateTag));
    tag->PutInt("Time", m_totalTickCount);
    tag->PutFloat("FallDistance", m_fallDistance);
}

Block *FallingBlock::GetBlock() const
{
    return m_block;
}

uint16_t FallingBlock::GetBlockData() const
{
    return m_blockData;
}

void FallingBlock::Land()
{
    m_level->RemoveEntity(this);

    if (m_level->IsClientSide())
    {
        if (dynamic_cast<GravityBlock *>(m_block))
        {
            return;
        }

        Block::UVRect rect = m_block->GetCrossAtlasUVRect(m_blockData);
        for (int i = 0; i < 16; i++)
        {
            double fx = m_random.NextFloat();
            double fy = m_random.NextFloat();
            double fz = m_random.NextFloat();
            ParticleSpawnParams params;
            params.type        = eParticleType_Terrain;
            params.position    = glm::dvec3(m_position.x - 0.5 + fx, m_position.y + fy, m_position.z - 0.5 + fz);
            params.velocity    = glm::dvec3(fx - 0.5, fy - 0.5, fz - 0.5);
            params.region      = {rect.u0, rect.v0, rect.u1, rect.v1};
            params.color       = 0xFF999999;
            params.hasVelocity = true;
            params.hasRegion   = true;
            params.hasColor    = true;
            m_level->SpawnParticle(params);
        }

        return;
    }

    PointedDripstoneBlock *pointed = dynamic_cast<PointedDripstoneBlock *>(m_block);
    if (pointed)
    {
        m_level->PlaySound(pointed->GetLandSound() + std::to_string(m_random.NextInt(1, pointed->GetLandSoundVariants())), m_position, 1.0f,
                           0.8f + m_random.NextFloat() * 0.4f);
    }

    AnvilBlock *anvil = dynamic_cast<AnvilBlock *>(m_block);
    int fallBlocks    = (int) ceilf(m_fallDistance - 1.0f);
    if (pointed && pointed->GetThickness(m_blockData) == PointedDripstoneBlock::eThickness_TIP && fallBlocks >= 0)
    {
        int damage                                                = std::min(fallBlocks * 6, 40);
        const std::vector<std::unique_ptr<ServerPlayer>> &players = ((ServerLevel *) m_level)->GetServer()->GetPlayerList()->GetPlayers();
        for (size_t i = 0; i < players.size(); i++)
        {
            if (players[i]->GetLevel() == m_level && players[i]->GetAABB().Intersects(m_aabb))
            {
                players[i]->Hurt(eDamageSource_FallingStalactite, damage, nullptr);
            }
        }
    }

    if (anvil && fallBlocks > 0)
    {
        int damage                                                = std::min(fallBlocks * 2, 40);
        const std::vector<std::unique_ptr<ServerPlayer>> &players = ((ServerLevel *) m_level)->GetServer()->GetPlayerList()->GetPlayers();
        for (size_t i = 0; i < players.size(); i++)
        {
            if (players[i]->GetLevel() == m_level && players[i]->GetAABB().Intersects(m_aabb))
            {
                players[i]->Hurt(eDamageSource_Anvil, damage, nullptr);
            }
        }

        const std::vector<std::unique_ptr<Entity>> &entities = m_level->GetEntities();
        for (size_t i = 0; i < entities.size(); i++)
        {
            LivingEntity *living = dynamic_cast<LivingEntity *>(entities[i].get());
            if (living && living->GetAABB().Intersects(m_aabb))
            {
                living->Hurt(eDamageSource_Anvil, damage, nullptr);
            }
        }

        if (m_random.NextFloat() < 0.05f + fallBlocks * 0.05f)
        {
            m_block = anvil->GetDamaged();
            if (!m_block)
            {
                m_level->PlaySound("random.anvil_break", m_position, 1.0f, 1.0f);
                return;
            }
        }
    }

    BlockPos pos((int) floor(m_position.x), (int) floor(m_position.y + 0.5), (int) floor(m_position.z));
    if (!m_level->IsAreaLoaded(pos, 0))
    {
        return;
    }

    while (pos.y < Chunk::s_sizeY && !GravityBlock::CanFallInto(Block::ById(m_level->GetBlockId(pos))))
    {
        pos.y++;
    }

    if (pos.y >= Chunk::s_sizeY)
    {
        return;
    }

    if (!dynamic_cast<GravityBlock *>(m_block) || !GravityBlock::CanFallThrough(Block::ById(m_level->GetBlockId(pos))))
    {
        Item *item = Item::ByName(m_block->GetResourceLocation());
        if (item)
        {
            std::unique_ptr<ItemEntity> itemEntity(new ItemEntity(m_level));
            itemEntity->Sync(glm::dvec3(pos.x + 0.5, pos.y + 0.25, pos.z + 0.5), glm::dvec3(0.0, 0.1, 0.0), ItemInstance(item, 1, 0), 0, 10);
            m_level->AddEntity(std::move(itemEntity));
        }

        return;
    }

    m_level->SetBlock(pos, m_block, nullptr, m_blockData);
    m_level->RecordBlockChange(pos);
    if (anvil)
    {
        m_level->PlaySound("random.anvil_land", glm::dvec3(pos.x + 0.5, pos.y + 0.5, pos.z + 0.5), 0.3f, 0.9f + m_random.NextFloat() * 0.1f);
    }
}
