#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Framebuffer.h"

#include "RenderDevice.h"
#include "Textures.h"

Framebuffer::Framebuffer(int width, int height, eTEXTURE_FORMAT colorFormat)
{
    m_colorFormat = colorFormat;
    m_width       = 0;
    m_height      = 0;

    Create(width, height);
}

Framebuffer::~Framebuffer()
{
    Destroy();
}

void Framebuffer::Resize(int width, int height)
{
    if (m_width == width && m_height == height)
    {
        return;
    }

    Destroy();
    Create(width, height);
}

void Framebuffer::Bind() const
{
    RenderDevice::GetInstance()->BindFramebuffer(m_fbo);
}

void Framebuffer::Unbind() const
{
    RenderDevice::GetInstance()->BindFramebuffer({});
}

FramebufferHandle Framebuffer::GetId() const
{
    return m_fbo;
}

TextureHandle Framebuffer::GetColorTexture() const
{
    return m_colorTexture;
}

TextureHandle Framebuffer::GetDepthTexture() const
{
    return m_depthTexture;
}

int Framebuffer::GetWidth() const
{
    return m_width;
}

int Framebuffer::GetHeight() const
{
    return m_height;
}

void Framebuffer::Create(int width, int height)
{
    m_width        = width;
    m_height       = height;
    m_colorTexture = Textures::Create(width, height, m_colorFormat, eTextureWrap_ClampToEdge, nullptr);
    m_depthTexture = Textures::Create(width, height, eTextureFormat_Depth32F, eTextureWrap_ClampToEdge, nullptr);
    m_fbo          = RenderDevice::GetInstance()->CreateFramebuffer(m_colorTexture, m_depthTexture);
}

void Framebuffer::Destroy()
{
    RenderDevice *device = RenderDevice::GetInstance();
    device->DeleteFramebuffer(m_fbo);
    device->DeleteTexture(m_depthTexture);
    device->DeleteTexture(m_colorTexture);
}

#endif
