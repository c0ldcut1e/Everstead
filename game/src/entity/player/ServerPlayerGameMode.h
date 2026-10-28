#pragma once

#include <cstdint>

#include "../../world/block/BlockPos.h"

class Block;
class Direction;
class ItemInstance;
class Level;
class ServerPlayer;

class ServerPlayerGameMode
{
public:
    ServerPlayerGameMode(ServerPlayer *player);

    void Tick();
    void StartDestroy(Level *level, const BlockPos &pos);
    void AbortDestroy();
    void DestroyBlock(Level *level, const BlockPos &pos);
    void UseBlock(Level *level, const BlockPos &pos, Direction *face);
    void UseItem();
    void ReleaseUsingItem();
    void UseItemOn(Level *level, const BlockPos &pos, Direction *face);
    void PlaceBlock(Level *level, const BlockPos &pos, Direction *face, uint16_t data);
    void ResendBlock(Level *level, const BlockPos &pos);
    bool BreakExtraBlock(Level *level, const BlockPos &pos, ItemInstance *held);

private:
    bool IsChunkLoaded(Level *level, const BlockPos &pos) const;
    bool CanInteract(Level *level, const BlockPos &pos) const;
    void UseSign(Level *level, const BlockPos &pos);
    void OpenSignEditor(Level *level, const BlockPos &pos, bool front);
    bool IntersectsAnyPlayer(const BlockPos &pos, Block *block, uint16_t data) const;
    void StopDestroy();
    void ConsumeHeldItem(ItemInstance *heldItem);
    void RejectPlace(Level *level, const BlockPos &pos);

    ServerPlayer *m_player;
    bool m_isDestroying;
    BlockPos m_destroyPos;
    float m_destroyProgress;
    bool m_hasDelayedDestroy;
    int m_lastDestroyStage;
    int m_tickCount;
};
