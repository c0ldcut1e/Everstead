#pragma once

#include "Block.h"

class EchoGlassBlock : public Block
{
public:
    EchoGlassBlock(const std::string &path, const std::string &solidPath);

    void ToggleFace(Level *level, const BlockPos &pos, Direction *face) const;
    UVRect GetBoxFaceAtlasUVRect(uint16_t data, const AABB &box, Direction *direction, bool *flipU) const override;

private:
    int m_solidProperty;
};
