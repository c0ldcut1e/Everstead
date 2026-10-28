#include "BufferedImage.h"

#include <cstring>
#include <stdexcept>
#include <string>

#include <stb_image.h>

BufferedImage::BufferedImage()
{
    m_pixelWidth  = 0;
    m_pixelHeight = 0;
}

BufferedImage::BufferedImage(const char *path)
{
    m_pixelWidth  = 0;
    m_pixelHeight = 0;

    Load(path);
}

BufferedImage::BufferedImage(int pixelWidth, int pixelHeight, const uint8_t *pixels)
{
    m_pixelWidth  = 0;
    m_pixelHeight = 0;

    SetPixels(pixelWidth, pixelHeight, pixels);
}

void BufferedImage::Load(const char *path)
{
    int width;
    int height;
    int channels;
    uint8_t *data = stbi_load(path, &width, &height, &channels, 4);
    if (!data)
    {
        throw std::runtime_error(std::string("failed to load image: ") + path);
    }

    SetPixels(width, height, data);
    stbi_image_free(data);
}

void BufferedImage::SetPixels(int pixelWidth, int pixelHeight, const uint8_t *pixels)
{
    if (pixelWidth <= 0 || pixelHeight <= 0 || !pixels)
    {
        throw std::runtime_error("failed to create buffered image");
    }

    m_pixelWidth  = pixelWidth;
    m_pixelHeight = pixelHeight;

    m_pixels.resize((size_t) pixelWidth * (size_t) pixelHeight * 4);
    memcpy(m_pixels.data(), pixels, m_pixels.size());
}

void BufferedImage::SamplePixel(int pixelX, int pixelY, float *r, float *g, float *b) const
{
    if (pixelX < 0)
    {
        pixelX = 0;
    }

    if (pixelY < 0)
    {
        pixelY = 0;
    }

    if (pixelX >= m_pixelWidth)
    {
        pixelX = m_pixelWidth - 1;
    }

    if (pixelY >= m_pixelHeight)
    {
        pixelY = m_pixelHeight - 1;
    }

    size_t offset = ((size_t) pixelY * (size_t) m_pixelWidth + (size_t) pixelX) * 4;
    *r            = (float) m_pixels[offset + 0] / 255.0f;
    *g            = (float) m_pixels[offset + 1] / 255.0f;
    *b            = (float) m_pixels[offset + 2] / 255.0f;
}

int BufferedImage::GetPixelWidth() const
{
    return m_pixelWidth;
}

int BufferedImage::GetPixelHeight() const
{
    return m_pixelHeight;
}

const std::vector<uint8_t> &BufferedImage::GetPixels() const
{
    return m_pixels;
}
