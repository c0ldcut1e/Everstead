#pragma once

#include <cstdint>
#include <vector>

#include "Block.h"

class RedstoneTorchBlock : public Block
{
public:
    RedstoneTorchBlock(bool lit, bool wall);

    void SetToggled(RedstoneTorchBlock *toggled);
    void OnPlace(Level *level, const BlockPos &pos) override;
    void OnRemoved(Level *level, const BlockPos &pos, uint16_t data) override;
    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void RandomTick(Level *level, const BlockPos &pos) override;
    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;
    int GetSignal(uint16_t data, Direction *direction) const override;
    int GetDirectSignal(uint16_t data, Direction *direction) const override;
    bool IsSignalSource() const override;

private:
    struct Toggle
    {
        Level *level;
        BlockPos pos;
        int64_t time;
    };

    bool IsToggledTooFrequently(Level *level, const BlockPos &pos, bool add);
    void NotifySignalNeighbors(Level *level, const BlockPos &pos);

    static std::vector<Toggle> s_toggles;

    bool m_lit;
    bool m_wall;
    RedstoneTorchBlock *m_toggled;
};
