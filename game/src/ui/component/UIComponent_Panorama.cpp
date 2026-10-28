#ifndef EVERSTEAD_DEDICATED_SERVER

#include "UIComponent_Panorama.h"

#include <cmath>
#include <sstream>

#include "../../core/Everstead.h"
#include "../../rendering/GlStateManager.h"
#include "../../rendering/Tesselator.h"
#include "../../rendering/Texture.h"
#include "../../rendering/TextureRepository.h"
#include "../../rendering/Textures.h"
#include "../../utils/Time.h"

UIComponent_Panorama::UIComponent_Panorama(const std::string &folderPath) : UIComponent()
{
    for (int i = 0; i < 6; i++)
    {
        std::stringstream stream;
        stream << folderPath << "/panorama_" << i << ".png";
        m_textures[i] = TextureRepository::GetInstance()->GetRaw(stream.str());
    }

    m_yaw            = 0.0f;
    m_pitchTime      = 0.0f;
    m_yawSpeed       = 1.8f;
    m_pitchSpeed     = 0.18f;
    m_pitchBase      = 20.0f;
    m_pitchAmplitude = 2.5f;
}

void UIComponent_Panorama::Render()
{
    for (int i = 0; i < 6; i++)
    {
        if (!m_textures[i] || m_textures[i]->GetPixelWidth() <= 0 || m_textures[i]->GetPixelHeight() <= 0)
        {
            return;
        }
    }

    Everstead *everstead = Everstead::GetInstance();
    float width          = (float) everstead->GetWidth();
    float height         = (float) everstead->GetHeight();
    float aspect         = width / height;
    float delta          = (float) Time::GetDelta();

    m_yaw += delta * m_yawSpeed;
    m_pitchTime += delta * m_pitchSpeed;

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::DisableBlend();
    GlStateManager::EnableTexture();

    GlStateManager::MatrixMode(1);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::Perspective(85.0f, aspect, 0.05f, 20.0f);

    GlStateManager::MatrixMode(0);
    GlStateManager::PushMatrix();
    GlStateManager::LoadIdentity();
    GlStateManager::Rotatef(m_pitchBase + sinf(m_pitchTime) * m_pitchAmplitude, 1.0f, 0.0f, 0.0f);
    GlStateManager::Rotatef(-m_yaw, 0.0f, 1.0f, 0.0f);

    for (int i = 0; i < 6; i++)
    {
        GlStateManager::PushMatrix();
        RotateFace(i);
        DrawFace(m_textures[i]);
        GlStateManager::PopMatrix();
    }

    GlStateManager::MatrixMode(0);
    GlStateManager::PopMatrix();

    GlStateManager::MatrixMode(1);
    GlStateManager::PopMatrix();

    GlStateManager::MatrixMode(1);
    GlStateManager::LoadIdentity();
    GlStateManager::Ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);

    GlStateManager::MatrixMode(0);
    GlStateManager::LoadIdentity();

    DrawDarkening(width, height);

    Textures::Unbind();

    GlStateManager::DisableBlend();
    GlStateManager::EnableTexture();
    GlStateManager::EnableCull();
    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void UIComponent_Panorama::SetYawSpeed(float yawSpeed)
{
    m_yawSpeed = yawSpeed;
}

void UIComponent_Panorama::SetPitchSpeed(float pitchSpeed)
{
    m_pitchSpeed = pitchSpeed;
}

void UIComponent_Panorama::SetPitchAmplitude(float pitchAmplitude)
{
    m_pitchAmplitude = pitchAmplitude;
}

void UIComponent_Panorama::RotateFace(int face)
{
    if (face == 1)
    {
        GlStateManager::Rotatef(90.0f, 0.0f, 1.0f, 0.0f);
    }
    else if (face == 2)
    {
        GlStateManager::Rotatef(180.0f, 0.0f, 1.0f, 0.0f);
    }
    else if (face == 3)
    {
        GlStateManager::Rotatef(-90.0f, 0.0f, 1.0f, 0.0f);
    }
    else if (face == 4)
    {
        GlStateManager::Rotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    }
    else if (face == 5)
    {
        GlStateManager::Rotatef(90.0f, 1.0f, 0.0f, 0.0f);
    }
}

void UIComponent_Panorama::DrawFace(Texture *texture)
{
    texture->SetWrapMode(eTextureWrap_ClampToEdge, eTextureWrap_ClampToEdge);

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    Textures::Bind(texture->GetId());
    builder->Color(0xFFFFFFFF);
    builder->VertexUV(-1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    builder->VertexUV(1.0f, -1.0f, 1.0f, 1.0f, 1.0f);
    builder->VertexUV(1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    builder->VertexUV(-1.0f, -1.0f, 1.0f, 0.0f, 1.0f);
    builder->VertexUV(1.0f, 1.0f, 1.0f, 1.0f, 0.0f);
    builder->VertexUV(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f);
    builder->End();
}

void UIComponent_Panorama::DrawDarkening(float width, float height)
{
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);
    GlStateManager::DisableTexture();

    BufferBuilder *builder = Tesselator::GetInstance()->GetBuilder();
    builder->Begin(ePrimitiveType_Triangles);
    builder->Color(0.0f, 0.0f, 0.0f, 0.34f);
    builder->Vertex(0.0f, 0.0f, 0.0f);
    builder->Vertex(width, 0.0f, 0.0f);
    builder->Vertex(width, height, 0.0f);
    builder->Vertex(0.0f, 0.0f, 0.0f);
    builder->Vertex(width, height, 0.0f);
    builder->Vertex(0.0f, height, 0.0f);
    builder->End();
}

#endif
