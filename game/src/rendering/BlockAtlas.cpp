#ifndef EVERSTEAD_DEDICATED_SERVER

#include "BlockAtlas.h"

#include <unordered_set>

#include "../utils/Direction.h"
#include "../world/block/BlockRegistry.h"

BlockAtlas *BlockAtlas::GetInstance()
{
    static BlockAtlas instance;
    return &instance;
}

void BlockAtlas::Build()
{
    BlockRegistry *registry = BlockRegistry::GetInstance();

    static Direction *directions[]       = {Direction::up, Direction::down, Direction::north, Direction::south, Direction::east, Direction::west};
    const std::string flowTexturePaths[] = {"assets/texture/block/water_flow.png", "assets/texture/block/lava_flow.png"};
    const char *flowBlockNames[]         = {"water_flow", "lava_flow"};

    std::vector<std::string> atlasPaths;
    std::unordered_set<std::string> seenPaths;

    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block = registry->ById(i);
        if (!block)
        {
            continue;
        }

        for (Direction *direction : directions)
        {
            const std::string &path = block->GetTexturePath(direction);
            if (path.empty() || path == flowTexturePaths[0] || path == flowTexturePaths[1])
            {
                continue;
            }

            if (seenPaths.insert(path).second)
            {
                atlasPaths.push_back(path);
            }
        }

        if (block->HasSideOverlay() && seenPaths.insert(block->GetSideOverlayPath()).second)
        {
            atlasPaths.push_back(block->GetSideOverlayPath());
        }

        if (block->HasSecondaryTexture() && seenPaths.insert(block->GetSecondaryTexturePath()).second)
        {
            atlasPaths.push_back(block->GetSecondaryTexturePath());
        }

        if (block->IsPillar() && seenPaths.insert(block->GetRotatedSidePath()).second)
        {
            atlasPaths.push_back(block->GetRotatedSidePath());
        }

        for (const std::string &variantPath : block->GetTextureVariantPaths())
        {
            if (seenPaths.insert(variantPath).second)
            {
                atlasPaths.push_back(variantPath);
            }
        }
    }

    m_blockAtlas.Build(atlasPaths);
    Texture *atlasTexture = m_blockAtlas.GetTexture();
    if (!atlasTexture)
    {
        return;
    }

    for (uint32_t i = 0; i < registry->Size(); i++)
    {
        Block *block = registry->ById(i);
        if (!block)
        {
            continue;
        }

        for (Direction *direction : directions)
        {
            const std::string &path = block->GetTexturePath(direction);
            if (path.empty() || !m_blockAtlas.Has(path))
            {
                continue;
            }

            TextureAtlas::Region region = m_blockAtlas.Get(path);
            block->SetTexture(direction, atlasTexture);
            block->SetAtlasUVRect(direction, region.u0, region.v0, region.u1, region.v1);
        }

        if (block->HasSideOverlay() && m_blockAtlas.Has(block->GetSideOverlayPath()))
        {
            TextureAtlas::Region region = m_blockAtlas.Get(block->GetSideOverlayPath());
            block->SetSideOverlayAtlasUVRect(region.u0, region.v0, region.u1, region.v1);
        }

        if (block->HasSecondaryTexture() && m_blockAtlas.Has(block->GetSecondaryTexturePath()))
        {
            TextureAtlas::Region region = m_blockAtlas.Get(block->GetSecondaryTexturePath());
            block->SetSecondaryAtlasUVRect(region.u0, region.v0, region.u1, region.v1);
        }

        if (block->IsPillar() && m_blockAtlas.Has(block->GetRotatedSidePath()))
        {
            TextureAtlas::Region region = m_blockAtlas.Get(block->GetRotatedSidePath());
            block->SetRotatedSideAtlasUVRect(region.u0, region.v0, region.u1, region.v1);
        }

        const std::vector<std::string> &variantPaths = block->GetTextureVariantPaths();
        for (int variant = 0; variant < (int) variantPaths.size(); variant++)
        {
            if (m_blockAtlas.Has(variantPaths[variant]))
            {
                TextureAtlas::Region region = m_blockAtlas.Get(variantPaths[variant]);
                block->SetTextureVariantAtlasUVRect(variant, region.u0, region.v0, region.u1, region.v1);
            }
        }
    }

    std::vector<std::string> flowPaths(std::begin(flowTexturePaths), std::end(flowTexturePaths));
    m_flowAtlas.Build(flowPaths);

    Texture *flowTexture = m_flowAtlas.GetTexture();
    for (int i = 0; i < 2; i++)
    {
        Block *flowBlock = Block::ByName(flowBlockNames[i]);
        if (!flowBlock || !flowTexture || !m_flowAtlas.Has(flowTexturePaths[i]))
        {
            continue;
        }

        TextureAtlas::Region flowRegion = m_flowAtlas.Get(flowTexturePaths[i]);
        for (Direction *direction : directions)
        {
            flowBlock->SetTexture(direction, flowTexture);
            flowBlock->SetAtlasUVRect(direction, flowRegion.u0, flowRegion.v0, flowRegion.u1, flowRegion.v1);
        }
    }
}

void BlockAtlas::TickAnimations(double deltaTime, const std::vector<TextureAtlas::Region> &visibleBlockRegions,
                                const std::vector<TextureAtlas::Region> &visibleFlowRegions)
{
    m_blockAtlas.UpdateAnimations(deltaTime, visibleBlockRegions);
    m_flowAtlas.UpdateAnimations(deltaTime, visibleFlowRegions);
}

TextureAtlas *BlockAtlas::GetBlockAtlas()
{
    return &m_blockAtlas;
}

TextureAtlas *BlockAtlas::GetFlowAtlas()
{
    return &m_flowAtlas;
}

#endif
