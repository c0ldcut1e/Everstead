#include "Explosion.h"

#include <algorithm>
#include <climits>
#include <cmath>
#include <numbers>
#include <string>
#include <unordered_set>

#include "../core/EversteadServer.h"
#include "../entity/LivingEntity.h"
#include "../entity/item/Minecart.h"
#include "../entity/player/ServerPlayer.h"
#include "../network/PlayerList.h"
#include "../network/listener/PlayerConnection.h"
#include "../network/packet/s2c/ExplodeS2CPacket.h"
#include "../utils/HitResult.h"
#include "../utils/math/Mth.h"
#include "Level.h"
#include "ServerLevel.h"
#include "block/Block.h"
#include "chunk/Chunk.h"
#include "particle/ParticleSpawnParams.h"

#include <glm/gtx/norm.hpp>

static bool IsExperimentBlast(eEXPLOSION_TYPE type)
{
    return type == eExplosionType_ExperimentFinale || type == eExplosionType_SingularityBlast || type == eExplosionType_NovaBlast ||
           type == eExplosionType_TempestBlast || type == eExplosionType_PrismBlast || type == eExplosionType_HellgateBlast;
}

Explosion::Explosion(Level *level, const glm::dvec3 &center, float radius)
{
    m_level  = level;
    m_center = center;
    m_radius = radius;
}

void Explosion::Explode()
{
    std::vector<BlockPos> blown = CollectBlocks();
    PushEntities(1.0, false, true);
    SendToPlayers(eExplosionType_Normal, blown);
    m_level->PlaySound("random.explode" + std::to_string(m_random.NextInt(1, 4)), m_center, 4.0f,
                       (1.0f + (m_random.NextFloat() - m_random.NextFloat()) * 0.2f) * 0.7f);
    DestroyBlocks(blown, 1.0f / m_radius);
}

void Explosion::Charge()
{
    SendToPlayers(eExplosionType_RiftCharge, {});
    m_level->PlaySound("fracture.collapse", m_center, 24.0f, 1.0f);
}

void Explosion::Collapse(const BlockPos &core)
{
    std::vector<BlockPos> blown = CollectBlocks();
    PushEntities(1.0, true, false);
    SendToPlayers(eExplosionType_RiftBurst, {});
    std::erase(blown, core);
    DestroyBlocks(blown, 0.3f);
}

void Explosion::Implode()
{
    PushEntities(-1.2, false, false);
    SendToPlayers(eExplosionType_RiftImplode, {});
}

void Explosion::Detonate(const BlockPos &core)
{
    std::vector<BlockPos> blown = CollectBlocks();
    PushEntities(4.5, true, false);
    SendToPlayers(eExplosionType_RiftDetonate, {});
    DestroyBlocks(blown, 0.1f);

    Block *air  = Block::ByName("air");
    Block *fire = Block::ByName("fire");
    for (size_t i = 0; i < blown.size(); i++)
    {
        const BlockPos &pos = blown[i];
        BlockPos below(pos.x, pos.y - 1, pos.z);
        if (pos.y > 0 && m_random.NextInt(0, 3) == 0 && Block::ById(m_level->GetBlockId(pos)) == air &&
            Block::ById(m_level->GetBlockId(below))->IsSolid())
        {
            m_level->SetBlock(pos, fire);
            m_level->RecordBlockChange(pos);
        }
    }

    std::vector<BlockPos> hole;
    BlockPos column((int) floor(m_center.x), (int) floor(m_center.y), (int) floor(m_center.z));
    for (int y = Chunk::s_sizeY - 1; y >= 0; y--)
    {
        float radius = m_radius * (0.2f + m_random.NextFloat() * 0.125f);
        int reach    = (int) ceil(radius);
        for (int x = -reach; x <= reach; x++)
        {
            for (int z = -reach; z <= reach; z++)
            {
                if ((float) (x * x + z * z) <= radius * radius)
                {
                    hole.push_back(BlockPos(column.x + x, y, column.z + z));
                }
            }
        }
    }

    std::erase(hole, core);
    DestroyBlocks(hole, 0.0f);
}

void Explosion::Experiment(eEXPLOSION_TYPE type, const BlockPos &core)
{
    std::vector<BlockPos> blown;
    if (type == eExplosionType_ExperimentBurst || IsExperimentBlast(type))
    {
        blown = CollectBlocks();
        std::erase(blown, core);
    }

    double strength = 3.0;
    if (type == eExplosionType_ExperimentBurst)
    {
        strength = 1.5;
    }
    else if (type == eExplosionType_ExperimentPull || type == eExplosionType_SingularityCharge || type == eExplosionType_HellgateCharge)
    {
        strength = -1.5;
    }
    else if (IsExperimentBlast(type))
    {
        strength = 6.0;
    }

    PushEntities(strength, false, false);
    SendToPlayers(type, {});
    DestroyBlocks(blown, 0.0f);
}

std::vector<BlockPos> Explosion::CollectBlocks()
{
    std::unordered_set<BlockPos> blownSet;
    std::vector<BlockPos> blown;
    Block *air = Block::ByName("air");
    int size   = 16;
    for (int xx = 0; xx < size; xx++)
    {
        for (int yy = 0; yy < size; yy++)
        {
            for (int zz = 0; zz < size; zz++)
            {
                if (xx != 0 && xx != size - 1 && yy != 0 && yy != size - 1 && zz != 0 && zz != size - 1)
                {
                    continue;
                }

                glm::dvec3 direction =
                        glm::normalize(glm::dvec3(xx / (size - 1.0) * 2.0 - 1.0, yy / (size - 1.0) * 2.0 - 1.0, zz / (size - 1.0) * 2.0 - 1.0));
                float power      = m_radius * (0.7f + m_random.NextFloat() * 0.6f);
                glm::dvec3 point = m_center;
                float stepSize   = 0.3f;
                ChunkPos chunkPos(INT_MAX, INT_MAX);
                const Chunk *chunk = nullptr;
                while (power > 0.0f)
                {
                    BlockPos pos((int) floor(point.x), (int) floor(point.y), (int) floor(point.z));
                    ChunkPos posChunk(Mth::FloorDiv(pos.x, Chunk::s_sizeX), Mth::FloorDiv(pos.z, Chunk::s_sizeZ));
                    if (posChunk != chunkPos)
                    {
                        chunkPos = posChunk;
                        chunk    = m_level->GetChunk(chunkPos);
                    }

                    if (!chunk)
                    {
                        break;
                    }

                    Block *block = pos.y < 0 || pos.y >= Chunk::s_sizeY ? air
                                                                        : Block::ById(chunk->GetBlockId(Mth::FloorMod(pos.x, Chunk::s_sizeX), pos.y,
                                                                                                        Mth::FloorMod(pos.z, Chunk::s_sizeZ)));
                    if (block != air)
                    {
                        power -= (block->GetExplosionResistance() + 0.3f) * stepSize;
                    }

                    if (power > 0.0f && blownSet.insert(pos).second)
                    {
                        blown.push_back(pos);
                    }

                    point += direction * (double) stepSize;
                    power -= stepSize * 0.75f;
                }
            }
        }
    }

    return blown;
}

void Explosion::PushEntities(double strength, bool halfHeart, bool blast)
{
    double reach = m_radius * 2.0;
    for (const std::unique_ptr<Entity> &entity : m_level->GetEntities())
    {
        double distance = sqrt(glm::distance2(entity->GetPosition(), m_center)) / reach;
        if (distance > 1.0)
        {
            continue;
        }

        glm::dvec3 away = entity->GetPosition() - m_center;
        if (glm::length2(away) > 0.0)
        {
            away = glm::normalize(away);
        }

        double pow = (1.0 - distance) * GetSeenPercent(entity->GetAABB());
        entity->SetVelocity(entity->GetVelocity() + away * (pow * strength));
        LivingEntity *living = dynamic_cast<LivingEntity *>(entity.get());
        if (halfHeart && living && living->GetHealth() > 1)
        {
            living->Hurt(eDamageSource_Rift, living->GetHealth() - 1, nullptr);
        }

        // Players take their blast damage in SendToPlayers
        if (blast && living && !dynamic_cast<Player *>(living))
        {
            living->Hurt(eDamageSource_Explosion, (int) ((pow * pow + pow) / 2.0 * 8.0 * reach + 1.0), nullptr);
        }

        Minecart *minecart = dynamic_cast<Minecart *>(entity.get());
        if (blast && pow > 0.0 && minecart && minecart->GetMinecartType() == Minecart::eMinecartType_Tnt)
        {
            minecart->PrimeFuse(m_level->GetRandom().NextInt(0, 19) + m_level->GetRandom().NextInt(0, 19));
        }
    }
}

void Explosion::SendToPlayers(eEXPLOSION_TYPE type, const std::vector<BlockPos> &blocks)
{
    double reach                                              = m_radius * 2.0;
    double range                                              = m_level->GetRenderDistance() * Chunk::s_sizeX;
    const std::vector<std::unique_ptr<ServerPlayer>> &players = ((ServerLevel *) m_level)->GetServer()->GetPlayerList()->GetPlayers();
    for (size_t i = 0; i < players.size(); i++)
    {
        ServerPlayer *player = players[i].get();
        if (player->GetLevel() != m_level || glm::distance2(player->GetPosition(), m_center) > range * range)
        {
            continue;
        }

        glm::dvec3 knockback;
        glm::dvec3 away = player->GetPosition() + glm::dvec3(0.0, player->GetEyeHeight(), 0.0) - m_center;
        if (glm::length2(away) > 0.0)
        {
            away = glm::normalize(away);
        }

        double distance = sqrt(glm::distance2(player->GetPosition(), m_center)) / reach;
        if (type == eExplosionType_RiftCharge && distance <= 0.25)
        {
            double angle = m_random.NextDouble() * std::numbers::pi * 2.0;
            knockback    = glm::dvec3(cos(angle) * 0.9, 0.45, sin(angle) * 0.9);
        }
        else if ((type == eExplosionType_RiftBurst || type == eExplosionType_RiftDetonate) && distance <= 1.0)
        {
            double pow = (1.0 - distance) * GetSeenPercent(player->GetAABB());
            if (player->GetHealth() > 1)
            {
                player->Hurt(eDamageSource_Rift, player->GetHealth() - 1, nullptr);
            }

            knockback = (type == eExplosionType_RiftDetonate ? away * (pow * 9.0) + glm::dvec3(0.0, pow * 2.5, 0.0)
                                                             : away * (pow * 2.0) + glm::dvec3(0.0, pow * 0.6, 0.0));
        }
        else if (type == eExplosionType_ExperimentLaunch && distance <= 1.0)
        {
            knockback = away * 3.5 + glm::dvec3(0.0, 1.4, 0.0);
        }
        else if (type == eExplosionType_ExperimentBurst && distance <= 1.0)
        {
            double pow = (1.0 - distance) * GetSeenPercent(player->GetAABB());
            knockback  = away * (pow * 2.0) + glm::dvec3(0.0, pow * 0.8, 0.0);
        }
        else if (IsExperimentBlast(type) && distance <= 1.0)
        {
            double pow = (1.0 - distance) * GetSeenPercent(player->GetAABB());
            knockback  = away * (pow * 10.0) + glm::dvec3(0.0, pow * 3.0, 0.0);
        }
        else if ((type == eExplosionType_RiftImplode || type == eExplosionType_ExperimentPull || type == eExplosionType_SingularityCharge ||
                  type == eExplosionType_HellgateCharge) &&
                 distance <= 1.0)
        {
            knockback = away * ((distance - 1.0) * 1.2);
        }
        else if (type == eExplosionType_Normal && distance <= 1.0)
        {
            double pow = (1.0 - distance) * GetSeenPercent(player->GetAABB());
            player->Hurt(eDamageSource_Explosion, (int) ((pow * pow + pow) / 2.0 * 8.0 * reach + 1.0), nullptr);
            knockback = away * pow;
        }

        player->GetPlayerConnection()->Send(std::shared_ptr<Packet>(new ExplodeS2CPacket(m_center, m_radius, blocks, knockback, type)));
    }
}

void Explosion::DestroyBlocks(const std::vector<BlockPos> &blocks, float dropChance)
{
    Block *air = Block::ByName("air");
    for (size_t i = blocks.size(); i-- > 0;)
    {
        const BlockPos &pos = blocks[i];
        Block *block        = Block::ById(m_level->GetBlockId(pos));
        if (block == air)
        {
            continue;
        }

        if (block->DropsFromExplosion() && m_random.NextFloat() < dropChance)
        {
            block->DropAsItem(m_level, pos, m_level->GetBlockData(pos));
        }

        m_level->SetBlock(pos, air);
        m_level->RecordBlockChange(pos);
        block->OnExploded(m_level, pos);
    }
}

void Explosion::SpawnParticles(const std::vector<BlockPos> &blocks)
{
    m_level->SpawnParticle(m_radius >= 2.0f ? eParticleType_HugeExplosion : eParticleType_LargeExplode, m_center);
    for (size_t i = 0; i < blocks.size(); i++)
    {
        const BlockPos &pos = blocks[i];
        glm::dvec3 point(pos.x + m_random.NextFloat(), pos.y + m_random.NextFloat(), pos.z + m_random.NextFloat());
        glm::dvec3 direction = point - m_center;
        double length        = glm::length(direction);
        double speed         = 0.5 / (length / m_radius + 0.1) * (m_random.NextFloat() * m_random.NextFloat() + 0.3f);
        ParticleSpawnParams params;
        params.type        = eParticleType_Explode;
        params.position    = (point + m_center) * 0.5;
        params.velocity    = direction * (speed / length);
        params.hasVelocity = true;
        m_level->SpawnParticle(params);
        params.type     = eParticleType_Smoke;
        params.position = point;
        m_level->SpawnParticle(params);
    }
}

// The charge sparks live exactly as long as the windup in collapse.ogg and drag 0.92 makes them cover about 10.4 times their speed
void Explosion::SpawnChargeParticles()
{
    ParticleSpawnParams params;
    params.type        = eParticleType_RiftSpark;
    params.hasVelocity = true;
    params.hasColor    = true;
    for (int i = 0; i < 400; i++)
    {
        glm::dvec3 direction = glm::normalize(glm::dvec3(m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5));
        glm::dvec3 swirl(-direction.z, 0.0, direction.x);
        double distance = 2.0 + m_random.NextDouble() * m_radius;
        params.position = m_center + direction * distance;
        params.velocity = (-direction + swirl * 0.35) * (distance / 10.4);
        params.color    = m_random.NextInt(0, 3) == 0 ? 0xFFF0D8FF : 0xFF7A28E0;
        params.scale    = 0.6f + m_random.NextFloat() * 1.2f;
        m_level->SpawnParticle(params);
    }
}

void Explosion::SpawnBurstParticles()
{
    ParticleSpawnParams params;
    params.type        = eParticleType_RiftSpark;
    params.position    = m_center;
    params.velocity    = glm::dvec3();
    params.hasVelocity = true;
    params.hasColor    = true;
    params.color       = 0xFFF4E8FF;
    params.scale       = 40.0f;
    m_level->SpawnParticle(params);

    params.type = eParticleType_Rift;
    for (int i = 0; i < 500; i++)
    {
        glm::dvec3 direction = glm::normalize(glm::dvec3(m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5));
        params.position      = m_center + direction * 0.5;
        params.velocity      = direction * (0.3 + m_random.NextDouble() * 1.4);
        params.color         = m_random.NextInt(0, 3) == 0 ? 0xFFF0D8FF : 0xFFA040FF;
        params.scale         = 1.0f + m_random.NextFloat() * 3.0f;
        m_level->SpawnParticle(params);
    }

    for (int i = 0; i < 240; i++)
    {
        double angle    = m_random.NextDouble() * std::numbers::pi * 2.0;
        params.position = m_center;
        params.velocity = glm::dvec3(cos(angle), (m_random.NextDouble() - 0.5) * 0.05, sin(angle)) * (1.8 + m_random.NextDouble() * 0.4);
        params.color    = 0xFFC890FF;
        params.scale    = 1.5f + m_random.NextFloat() * 1.5f;
        m_level->SpawnParticle(params);
    }

    for (int i = 0; i < 400; i++)
    {
        double angle    = m_random.NextDouble() * std::numbers::pi * 2.0;
        double distance = m_random.NextDouble() * m_radius * 0.5;
        params.position = m_center + glm::dvec3(cos(angle) * distance, -m_random.NextDouble() * 30.0, sin(angle) * distance);
        params.velocity = glm::dvec3(0.0, -0.2 - m_random.NextDouble() * 0.6, 0.0);
        params.color    = 0xFF8030E8;
        params.scale    = 0.8f + m_random.NextFloat() * 2.0f;
        m_level->SpawnParticle(params);
    }

    for (int i = 0; i < 300; i++)
    {
        glm::dvec3 offset(m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5);
        params.position = m_center + offset * (m_radius * 3.0);
        params.velocity = glm::dvec3(0.0, 0.01 + m_random.NextDouble() * 0.03, 0.0);
        params.color    = 0xFF9A50F0;
        params.scale    = 0.5f + m_random.NextFloat();
        m_level->SpawnParticle(params);
    }
}

void Explosion::SpawnImplodeParticles()
{
    ParticleSpawnParams params;
    params.type        = eParticleType_RiftSpark;
    params.hasVelocity = true;
    params.hasColor    = true;
    for (int i = 0; i < 600; i++)
    {
        glm::dvec3 direction = glm::normalize(glm::dvec3(m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5));
        glm::dvec3 swirl(-direction.z, 0.0, direction.x);
        double distance = 4.0 + m_random.NextDouble() * m_radius * 2.0;
        params.position = m_center + direction * distance;
        params.velocity = (-direction + swirl * 0.5) * (distance / 9.0);
        params.color    = m_random.NextInt(0, 3) == 0 ? 0xFFFFE0C0 : 0xFF5018C0;
        params.scale    = 0.8f + m_random.NextFloat() * 1.6f;
        m_level->SpawnParticle(params);
    }
}

void Explosion::SpawnExperimentParticles(eEXPLOSION_TYPE type)
{
    ParticleSpawnParams params;
    params.type        = eParticleType_RiftSpark;
    params.hasVelocity = true;
    params.hasColor    = true;
    bool blast         = IsExperimentBlast(type);
    bool pull          = type == eExplosionType_ExperimentPull || type == eExplosionType_SingularityCharge || type == eExplosionType_HellgateCharge;
    int count          = blast ? 2400 : (type == eExplosionType_ExperimentBurst ? 450 : 900);
    double speed       = blast ? 2.4 : 1.0;
    uint32_t palettes[][2] = {{0xFFFFB347, 0xFF7A3CFF}, {0xFFFFF0A0, 0xFFFF3A10}, {0xFFE8F4FF, 0xFF3A6BFF}, {0, 0}, {0xFFFF2A0A, 0xFF300004}};
    int palette            = type >= eExplosionType_SingularityCharge && type != eExplosionType_PrismCharge && type != eExplosionType_PrismBlast
                                     ? (type - eExplosionType_SingularityCharge) / 2
                                     : -1;
    if (!pull)
    {
        params.position = m_center;
        params.velocity = glm::dvec3();
        params.color    = 0xFFFFFFFF;
        params.scale    = m_radius * 3.0f;
        m_level->SpawnParticle(params);
    }

    for (int i = 0; i < count; i++)
    {
        glm::dvec3 direction = glm::normalize(glm::dvec3(m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5, m_random.NextDouble() - 0.5));
        double hue           = (double) i / count * std::numbers::pi * 2.0;
        params.color         = 0xFF000000 | (uint32_t) (127.5 + 127.5 * cos(hue)) << 16 | (uint32_t) (127.5 + 127.5 * cos(hue - 2.094)) << 8 |
                               (uint32_t) (127.5 + 127.5 * cos(hue - 4.189));
        if (palette >= 0)
        {
            double mix   = 0.5 + 0.5 * cos(hue * 3.0);
            params.color = 0xFF000000;
            for (int shift = 0; shift < 24; shift += 8)
            {
                double a = (palettes[palette][0] >> shift) & 0xFF;
                double b = (palettes[palette][1] >> shift) & 0xFF;
                params.color |= (uint32_t) (a + (b - a) * mix) << shift;
            }
        }

        params.scale = 0.8f + m_random.NextFloat() * 2.0f;
        if (pull)
        {
            glm::dvec3 swirl(-direction.z, 0.0, direction.x);
            double distance = 4.0 + m_random.NextDouble() * m_radius * 2.0;
            params.position = m_center + direction * distance;
            params.velocity = (-direction + swirl * 0.8) * (distance / 12.0);
        }
        else if (i % 5 == 0)
        {
            params.position = m_center;
            params.velocity = glm::dvec3(direction.x * 0.15, 1.2 + m_random.NextDouble() * 1.6, direction.z * 0.15) * speed;
        }
        else if (i % 3 == 0)
        {
            double angle    = hue * 3.0;
            params.position = m_center;
            params.velocity = glm::dvec3(cos(angle), (m_random.NextDouble() - 0.5) * 0.1, sin(angle)) * (1.5 + m_random.NextDouble() * 0.5) * speed;
        }
        else
        {
            params.position = m_center + direction * 0.5;
            params.velocity = direction * (0.3 + m_random.NextDouble() * 1.4) * speed;
        }

        m_level->SpawnParticle(params);
    }
}

float Explosion::GetSeenPercent(const AABB &box)
{
    const glm::dvec3 &min = box.GetMin();
    const glm::dvec3 &max = box.GetMax();
    glm::dvec3 step(1.0 / ((max.x - min.x) * 2.0 + 1.0), 1.0 / ((max.y - min.y) * 2.0 + 1.0), 1.0 / ((max.z - min.z) * 2.0 + 1.0));
    int hits  = 0;
    int count = 0;
    for (double xx = 0.0; xx <= 1.0; xx += step.x)
    {
        for (double yy = 0.0; yy <= 1.0; yy += step.y)
        {
            for (double zz = 0.0; zz <= 1.0; zz += step.z)
            {
                glm::dvec3 point(min.x + (max.x - min.x) * xx, min.y + (max.y - min.y) * yy, min.z + (max.z - min.z) * zz);
                glm::dvec3 toCenter = m_center - point;
                // We stop just short of the center because it sits on the ground plane and the ray would count the block under it
                HitResult *result = m_level->Clip(point, toCenter, (float) glm::length(toCenter) - 0.001f, false, false, true);
                if (!result->IsBlock())
                {
                    hits++;
                }

                delete result;
                count++;
            }
        }
    }

    return hits / (float) count;
}
