#pragma once

#include <cstdint>
#include <vector>

#include "BufferedImage.h"
#include "RenderTypes.h"

class Texture
{
public:
    explicit Texture(const BufferedImage &image);
    ~Texture();

    void Reload(const BufferedImage &image);
    void SamplePixel(int pixelX, int pixelY, float *r, float *g, float *b) const;
    void SetWrapMode(eTEXTURE_WRAP wrapS, eTEXTURE_WRAP wrapT) const;
    void SetFilterMode(eTEXTURE_FILTER minFilter, eTEXTURE_FILTER magFilter) const;

    TextureHandle GetId() const;

    int GetPixelWidth() const;
    int GetPixelHeight() const;
    const BufferedImage &GetImage() const;
    const std::vector<uint8_t> &GetPixels() const;

private:
    TextureHandle m_id;
    BufferedImage m_image;
};
