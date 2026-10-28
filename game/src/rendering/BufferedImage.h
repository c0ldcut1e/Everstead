#pragma once

#include <cstdint>
#include <vector>

class BufferedImage
{
public:
    BufferedImage();
    explicit BufferedImage(const char *path);
    BufferedImage(int pixelWidth, int pixelHeight, const uint8_t *pixels);

    void Load(const char *path);
    void SetPixels(int pixelWidth, int pixelHeight, const uint8_t *pixels);
    void SamplePixel(int pixelX, int pixelY, float *r, float *g, float *b) const;

    int GetPixelWidth() const;
    int GetPixelHeight() const;
    const std::vector<uint8_t> &GetPixels() const;

private:
    std::vector<uint8_t> m_pixels;
    int m_pixelWidth;
    int m_pixelHeight;
};
