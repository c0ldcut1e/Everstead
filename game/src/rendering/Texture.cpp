#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Texture.h"

#include "RenderDevice.h"
#include "Textures.h"

Texture::Texture(const BufferedImage &image)
{
    m_image = image;
    m_id    = Textures::Create(m_image.GetPixelWidth(), m_image.GetPixelHeight(), eTextureFormat_RGBA8, eTextureWrap_ClampToEdge,
                               m_image.GetPixels().data());
}

Texture::~Texture()
{
    // Static atlases outlive the render device, and their textures were already freed with it
    RenderDevice *device = RenderDevice::GetInstance();
    if (device)
    {
        device->DeleteTexture(m_id);
    }
}

void Texture::Reload(const BufferedImage &image)
{
    RenderDevice::GetInstance()->DeleteTexture(m_id);
    m_image = image;
    m_id    = Textures::Create(m_image.GetPixelWidth(), m_image.GetPixelHeight(), eTextureFormat_RGBA8, eTextureWrap_ClampToEdge,
                               m_image.GetPixels().data());
}

void Texture::SamplePixel(int pixelX, int pixelY, float *r, float *g, float *b) const
{
    m_image.SamplePixel(pixelX, pixelY, r, g, b);
}

void Texture::SetWrapMode(eTEXTURE_WRAP wrapS, eTEXTURE_WRAP wrapT) const
{
    RenderDevice::GetInstance()->SetTextureWrap(m_id, wrapS, wrapT);
}

void Texture::SetFilterMode(eTEXTURE_FILTER minFilter, eTEXTURE_FILTER magFilter) const
{
    RenderDevice::GetInstance()->SetTextureFilter(m_id, minFilter, magFilter);
}

TextureHandle Texture::GetId() const
{
    return m_id;
}

int Texture::GetPixelWidth() const
{
    return m_image.GetPixelWidth();
}

int Texture::GetPixelHeight() const
{
    return m_image.GetPixelHeight();
}

const BufferedImage &Texture::GetImage() const
{
    return m_image;
}

const std::vector<uint8_t> &Texture::GetPixels() const
{
    return m_image.GetPixels();
}

#endif
