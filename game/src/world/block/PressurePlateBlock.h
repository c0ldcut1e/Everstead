#pragma once

#include "SwitchBlock.h"

class PressurePlateBlock : public SwitchBlock
{
public:
    PressurePlateBlock(const std::string &texturePath);

    void GetBoxes(uint16_t data, std::vector<AABB> *out) const override;
    void Tick(Level *level, const BlockPos &pos) override;

    // Entities standing on the plate call this
    void OnEntityInside(Level *level, const BlockPos &pos);

private:
    static constexpr int kCheckTicks = 20;

    bool HasEntityOnTop(Level *level, const BlockPos &pos) const;
    void SetPressed(Level *level, const BlockPos &pos, bool pressed);
};
