#pragma once

#include <string>

#include "Block.h"

class DispenserBlockEntity;

class DispenserBlock : public Block
{
public:
    DispenserBlock(const std::string &frontPath, const std::string &verticalFrontPath);

    void OnNeighborChanged(Level *level, const BlockPos &pos) override;
    void Tick(Level *level, const BlockPos &pos) override;
    void OnBreak(Level *level, const BlockPos &pos) override;
    UVRect GetAtlasUVRect(Direction *direction, uint16_t data) const override;
    uint16_t GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const override;
    uint16_t GetItemData() const override;
    bool IsUsable(uint16_t data) const override;
    const char *GetBlockEntityId() const override;
    bool HasAnalogOutput() const override;
    int GetAnalogOutput(Level *level, const BlockPos &pos) const override;

    Direction *GetFacing(uint16_t data) const;

protected:
    virtual void DispenseFrom(Level *level, const BlockPos &pos, DispenserBlockEntity *dispenser, int slot);

private:
    int m_facingProperty;
    int m_triggeredProperty;
    int m_frontTexture;
    int m_verticalFrontTexture;
    int m_topTexture;
};
