#pragma once

#include "DiodeBlock.h"

class RepeaterBlock : public DiodeBlock
{
public:
    RepeaterBlock();

    void OnPlace(Level *level, const BlockPos &pos) override;
    void AnimateTick(Level *level, const BlockPos &pos, Random *random) override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    bool CanConnectRedstone(uint16_t data, Direction *direction) const override;
    void GetTorches(uint16_t data, std::vector<glm::dvec3> *out, std::vector<bool> *lit) const override;

protected:
    void GetElements(uint16_t data, std::vector<Element> *out) const override;
    void CheckTickOnNeighbor(Level *level, const BlockPos &pos, uint16_t data) override;
    bool IsLocked(Level *level, const BlockPos &pos, uint16_t data) const override;
    int GetDelay(uint16_t data) const override;
    int GetOutputSignal(uint16_t data) const override;
    bool IsAlternateInput(Block *block) const override;

private:
    int m_delayProperty;
    int m_lockedProperty;
    int m_lockTexture;
};
