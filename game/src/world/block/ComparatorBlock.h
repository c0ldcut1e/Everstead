#pragma once

#include "DiodeBlock.h"

class ComparatorBlock : public DiodeBlock
{
public:
    ComparatorBlock();

    void Tick(Level *level, const BlockPos &pos) override;
    bool OnUse(Level *level, const BlockPos &pos) override;
    void GetTorches(uint16_t data, std::vector<glm::dvec3> *out, std::vector<bool> *lit) const override;

protected:
    void GetElements(uint16_t data, std::vector<Element> *out) const override;
    void CheckTickOnNeighbor(Level *level, const BlockPos &pos, uint16_t data) override;
    bool ShouldTurnOn(Level *level, const BlockPos &pos, uint16_t data) const override;
    int GetDelay(uint16_t data) const override;
    int GetOutputSignal(uint16_t data) const override;
    int GetInputSignal(Level *level, const BlockPos &pos, uint16_t data) const override;

private:
    bool IsSubtract(uint16_t data) const;
    int CalculateOutput(Level *level, const BlockPos &pos, uint16_t data) const;

    int m_modeProperty;
    int m_outputProperty;
};
