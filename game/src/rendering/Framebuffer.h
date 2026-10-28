#pragma once

#include "RenderTypes.h"

class Framebuffer
{
public:
    Framebuffer(int width, int height, eTEXTURE_FORMAT colorFormat = eTextureFormat_RGBA8);
    ~Framebuffer();

    void Resize(int width, int height);

    void Bind() const;
    void Unbind() const;

    FramebufferHandle GetId() const;
    TextureHandle GetColorTexture() const;
    TextureHandle GetDepthTexture() const;

    int GetWidth() const;
    int GetHeight() const;

private:
    void Create(int width, int height);
    void Destroy();

    FramebufferHandle m_fbo;
    TextureHandle m_colorTexture;
    TextureHandle m_depthTexture;
    eTEXTURE_FORMAT m_colorFormat;

    int m_width;
    int m_height;
};
