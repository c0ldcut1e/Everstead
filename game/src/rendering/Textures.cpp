#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Textures.h"

#include "GlStateManager.h"
#include "RenderDevice.h"

TextureHandle Textures::s_boundTexture;

TextureHandle Textures::Create(int width, int height, eTEXTURE_FORMAT format, eTEXTURE_WRAP wrap, const void *pixels)
{
    RenderDevice *device = RenderDevice::GetInstance();
    TextureHandle handle = device->CreateTexture();
    device->UploadTexture(handle, 0, width, height, format, pixels);
    device->SetTextureFilter(handle, eTextureFilter_Nearest, eTextureFilter_Nearest);
    device->SetTextureWrap(handle, wrap, wrap);
    return handle;
}

void Textures::UploadAlphaSafeMipmaps(TextureHandle handle, const std::vector<uint8_t> &basePixels, int cellWidth, int cellHeight, int columns,
                                      int rows, int levelCount)
{
    while (levelCount > 1 && (cellWidth % (1 << (levelCount - 1)) != 0 || cellHeight % (1 << (levelCount - 1)) != 0))
    {
        levelCount--;
    }

    RenderDevice *device      = RenderDevice::GetInstance();
    std::vector<uint8_t> prev = basePixels;
    int prevCellWidth         = cellWidth;
    int prevCellHeight        = cellHeight;

    for (int level = 1; level < levelCount; level++)
    {
        int prevWidth     = columns * prevCellWidth;
        int dstCellWidth  = prevCellWidth / 2;
        int dstCellHeight = prevCellHeight / 2;
        int dstWidth      = columns * dstCellWidth;
        int dstHeight     = rows * dstCellHeight;
        std::vector<uint8_t> current((size_t) dstWidth * (size_t) dstHeight * 4, 0);

        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < columns; col++)
            {
                int srcCellX = col * prevCellWidth;
                int srcCellY = row * prevCellHeight;
                int dstCellX = col * dstCellWidth;
                int dstCellY = row * dstCellHeight;

                for (int cy = 0; cy < dstCellHeight; cy++)
                {
                    for (int cx = 0; cx < dstCellWidth; cx++)
                    {
                        int sumR = 0;
                        int sumG = 0;
                        int sumB = 0;
                        int sumA = 0;
                        for (int dy = 0; dy < 2; dy++)
                        {
                            for (int dx = 0; dx < 2; dx++)
                            {
                                int sx    = srcCellX + cx * 2 + dx;
                                int sy    = srcCellY + cy * 2 + dy;
                                size_t so = ((size_t) sy * (size_t) prevWidth + (size_t) sx) * 4;
                                int a     = prev[so + 3];
                                sumR += prev[so + 0] * a;
                                sumG += prev[so + 1] * a;
                                sumB += prev[so + 2] * a;
                                sumA += a;
                            }
                        }

                        size_t doff = ((size_t) (dstCellY + cy) * (size_t) dstWidth + (size_t) (dstCellX + cx)) * 4;
                        if (sumA > 0)
                        {
                            current[doff + 0] = (uint8_t) (sumR / sumA);
                            current[doff + 1] = (uint8_t) (sumG / sumA);
                            current[doff + 2] = (uint8_t) (sumB / sumA);
                        }

                        current[doff + 3] = (uint8_t) (sumA / 4);
                    }
                }
            }
        }

        device->UploadTexture(handle, level, dstWidth, dstHeight, eTextureFormat_RGBA8, current.data());
        prev           = std::move(current);
        prevCellWidth  = dstCellWidth;
        prevCellHeight = dstCellHeight;
    }

    device->SetTextureMaxLevel(handle, levelCount - 1);
    device->SetTextureFilter(handle, eTextureFilter_NearestMipmapNearest, eTextureFilter_Nearest);
}

void Textures::Bind(TextureHandle handle, uint32_t slot)
{
    if (slot == 0)
    {
        s_boundTexture = handle;
    }

    RenderDevice::GetInstance()->BindTexture(handle, slot);

    if (!handle.IsValid())
    {
        GlStateManager::DisableTexture();
        return;
    }

    GlStateManager::EnableTexture();
}

void Textures::Unbind()
{
    Bind({});
}

TextureHandle Textures::GetBound()
{
    return s_boundTexture;
}

#endif
