#ifndef EVERSTEAD_DEDICATED_SERVER

#include "ChunkMesherGreedyMesher.h"
#include "ChunkMesherCache.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "../../utils/math/Mth.h"
#include "../LevelSource.h"
#include "../lighting/LightEngine.h"

void ChunkMesherGreedyMesher::Greedy2D(std::vector<MaskCell> &mask, int width, int height, bool useGreedy, std::vector<GreedyRect> *outRects)
{
    if (useGreedy && width > 0 && width <= 64 && height > 0)
    {
        thread_local std::vector<uint64_t> rowMasks;
        rowMasks.assign((size_t) height, 0);
        for (int j = 0; j < height; j++)
        {
            uint64_t bits = 0;
            for (int i = 0; i < width; i++)
            {
                if (mask[(size_t) i + (size_t) width * (size_t) j].filled)
                {
                    bits |= (uint64_t) 1 << i;
                }
            }

            rowMasks[(size_t) j] = bits;
        }

        for (int j = 0; j < height; j++)
        {
            while (rowMasks[(size_t) j] != 0)
            {
                int i         = (int) std::countr_zero(rowMasks[(size_t) j]);
                MaskCell cell = mask[(size_t) i + (size_t) width * (size_t) j];
                if (!cell.filled || !cell.texture)
                {
                    rowMasks[(size_t) j] &= ~((uint64_t) 1 << i);
                    continue;
                }

                int rWidth = 1;
                while (i + rWidth < width)
                {
                    uint64_t bit = (uint64_t) 1 << (i + rWidth);
                    if ((rowMasks[(size_t) j] & bit) == 0)
                    {
                        break;
                    }

                    MaskCell nextCell = mask[(size_t) (i + rWidth) + (size_t) width * (size_t) j];
                    if (!ChunkMesherCache::CanMergeMaskCells(cell, nextCell) ||
                        !ChunkMesherCache::CanRepresentSmoothLightRect(mask, width, i, j, rWidth + 1, 1))
                    {
                        break;
                    }

                    rWidth++;
                }

                int rHeight = 1;
                bool expand = true;
                while (expand && j + rHeight < height)
                {
                    int row = j + rHeight;
                    for (int k = 0; k < rWidth; k++)
                    {
                        int col      = i + k;
                        uint64_t bit = (uint64_t) 1 << col;
                        if ((rowMasks[(size_t) row] & bit) == 0)
                        {
                            expand = false;
                            break;
                        }

                        MaskCell nextCell = mask[(size_t) col + (size_t) width * (size_t) row];
                        if (!ChunkMesherCache::CanMergeMaskCells(cell, nextCell))
                        {
                            expand = false;
                            break;
                        }
                    }

                    if (expand && !ChunkMesherCache::CanRepresentSmoothLightRect(mask, width, i, j, rWidth, rHeight + 1))
                    {
                        expand = false;
                    }

                    if (expand)
                    {
                        rHeight++;
                    }
                }

                GreedyRect rect;
                rect.x          = i;
                rect.y          = j;
                rect.width      = rWidth;
                rect.height     = rHeight;
                rect.texture    = cell.texture;
                rect.atlasRect  = cell.atlasRect;
                rect.rawLight   = cell.rawLight;
                rect.tint       = cell.tint;
                rect.shapeFlags = cell.shapeFlags;
                outRects->push_back(rect);

                uint64_t widthBits = 0;
                if (rWidth >= 64)
                {
                    widthBits = UINT64_MAX;
                }
                else
                {
                    widthBits = ((uint64_t) 1 << rWidth) - 1;
                }

                uint64_t rangeBits = widthBits << i;

                for (int y = j; y < j + rHeight; y++)
                {
                    rowMasks[(size_t) y] &= ~rangeBits;
                    for (int x = i; x < i + rWidth; x++)
                    {
                        mask[(size_t) x + (size_t) width * (size_t) y].filled = false;
                    }
                }
            }
        }

        return;
    }

    for (int j = 0; j < height; j++)
    {
        for (int i = 0; i < width; i++)
        {
            MaskCell cell = mask[(size_t) i + (size_t) width * (size_t) j];
            if (!cell.filled || !cell.texture)
            {
                continue;
            }

            int rWidth = 1;
            while (useGreedy && i + rWidth < width)
            {
                MaskCell nextCell = mask[(size_t) (i + rWidth) + (size_t) width * (size_t) j];
                if (!ChunkMesherCache::CanMergeMaskCells(cell, nextCell) ||
                    !ChunkMesherCache::CanRepresentSmoothLightRect(mask, width, i, j, rWidth + 1, 1))
                {
                    break;
                }

                rWidth++;
            }

            int rHeight = 1;
            bool expand = useGreedy;
            while (expand && j + rHeight < height)
            {
                for (int k = 0; k < rWidth; k++)
                {
                    MaskCell nextCell = mask[(size_t) (i + k) + (size_t) width * (size_t) (j + rHeight)];
                    if (!ChunkMesherCache::CanMergeMaskCells(cell, nextCell))
                    {
                        expand = false;
                        break;
                    }
                }

                if (expand && !ChunkMesherCache::CanRepresentSmoothLightRect(mask, width, i, j, rWidth, rHeight + 1))
                {
                    expand = false;
                }

                if (expand)
                {
                    rHeight++;
                }
            }

            for (int y = 0; y < rHeight; y++)
            {
                for (int x = 0; x < rWidth; x++)
                {
                    size_t idx       = (size_t) (i + x) + (size_t) width * (size_t) (j + y);
                    mask[idx].filled = false;
                }
            }

            GreedyRect rect;
            rect.x          = i;
            rect.y          = j;
            rect.width      = rWidth;
            rect.height     = rHeight;
            rect.texture    = cell.texture;
            rect.atlasRect  = cell.atlasRect;
            rect.rawLight   = cell.rawLight;
            rect.tint       = cell.tint;
            rect.shapeFlags = cell.shapeFlags;
            outRects->push_back(rect);
        }
    }
}

#endif
