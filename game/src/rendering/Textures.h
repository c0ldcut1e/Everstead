#pragma once

#include <cstdint>
#include <vector>

#include "RenderTypes.h"

class Textures
{
public:
    static TextureHandle Create(int width, int height, eTEXTURE_FORMAT format, eTEXTURE_WRAP wrap, const void *pixels);
    static void UploadAlphaSafeMipmaps(TextureHandle handle, const std::vector<uint8_t> &basePixels, int cellWidth, int cellHeight, int columns,
                                       int rows, int levelCount);
    static void Bind(TextureHandle handle, uint32_t slot = 0);
    static void Unbind();
    static TextureHandle GetBound();

private:
    static TextureHandle s_boundTexture;
};
