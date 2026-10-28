#include "LayerBlock.h"

#include <algorithm>

#include "../Level.h"

LayerBlock::LayerBlock(const std::string &texturePath, Block *fullBlock) : Block(true, texturePath)
{
    m_fullBlock      = fullBlock;
    m_layersProperty = AddProperty("layers", kMaxLayers);
    SetTransparent(true);
    SetRenderShape(eRenderShape_BOXES);
    SetNeedsSupportBelow(true);
    SetLiquidReplaceable(true);
}

void LayerBlock::GetBoxes(uint16_t data, std::vector<AABB> *out) const
{
    double height = (double) GetLayers(data) / (double) kMaxLayers;
    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, height, 1.0)));
}

void LayerBlock::GetCollisionBoxes(uint16_t data, std::vector<AABB> *out) const
{
    int layers = GetLayers(data) - 1;
    if (layers <= 0)
    {
        return;
    }

    out->push_back(AABB(glm::dvec3(0.0, 0.0, 0.0), glm::dvec3(1.0, (double) layers / (double) kMaxLayers, 1.0)));
}

bool LayerBlock::CanBeReplaced(uint16_t data) const
{
    return GetLayers(data) == 1;
}

uint16_t LayerBlock::GetPlacementState(Level *level, const BlockPos &pos, Direction *face, const glm::dvec3 &hitPos, const glm::dvec3 &look) const
{
    (void) face;
    (void) hitPos;
    (void) look;

    if (Block::ById(level->GetBlockId(pos)) != this)
    {
        return 0;
    }

    return WithLayers(std::min(GetLayers(level->GetBlockData(pos)) + 1, kMaxLayers));
}

Block *LayerBlock::GetCombinedBlock(uint16_t existingData, uint16_t placedData) const
{
    int existing = GetLayers(existingData);
    int placed   = GetLayers(placedData);
    if (placed <= existing || existing >= kMaxLayers)
    {
        return nullptr;
    }

    return placed >= kMaxLayers ? m_fullBlock : (Block *) this;
}

int LayerBlock::GetLayers(uint16_t data) const
{
    return GetPropertyValue(data, m_layersProperty) + 1;
}

uint16_t LayerBlock::WithLayers(int layers) const
{
    return WithPropertyValue(0, m_layersProperty, layers - 1);
}
