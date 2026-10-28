#ifndef EVERSTEAD_DEDICATED_SERVER

#include "GlStateManager.h"

#include "RenderDevice.h"

#include <glm/gtc/matrix_transform.hpp>

std::vector<glm::dmat4> GlStateManager::s_modelViewStack;
std::vector<glm::dmat4> GlStateManager::s_projectionStack;
glm::dmat4 GlStateManager::s_modelViewMatrix  = glm::dmat4(1.0);
glm::dmat4 GlStateManager::s_projectionMatrix = glm::dmat4(1.0);
int GlStateManager::s_matrixMode              = 0;

bool GlStateManager::s_depthTest          = true;
bool GlStateManager::s_depthMask          = true;
eCOMPARE_FUNC GlStateManager::s_depthFunc = eCompareFunc_Less;

bool GlStateManager::s_cull             = false;
eCULL_FACE GlStateManager::s_cullFace   = eCullFace_Back;
eFRONT_FACE GlStateManager::s_frontFace = eFrontFace_Ccw;

bool GlStateManager::s_blend             = false;
eBLEND_FACTOR GlStateManager::s_blendSrc = eBlendFactor_SrcAlpha;
eBLEND_FACTOR GlStateManager::s_blendDst = eBlendFactor_OneMinusSrcAlpha;

bool GlStateManager::s_scissor        = false;
bool GlStateManager::s_textureEnabled = false;
float GlStateManager::s_alphaCutoff   = -1.0f;

bool GlStateManager::s_fog              = true;
float GlStateManager::s_fogR            = 0.47058824f;
float GlStateManager::s_fogG            = 0.65490198f;
float GlStateManager::s_fogB            = 1.0f;
float GlStateManager::s_fogStart        = 0.0f;
float GlStateManager::s_fogEnd          = 0.0f;
uint32_t GlStateManager::s_overlayColor = 0;
float GlStateManager::s_glint           = 0.0f;

bool GlStateManager::s_colorMaskR = true;
bool GlStateManager::s_colorMaskG = true;
bool GlStateManager::s_colorMaskB = true;
bool GlStateManager::s_colorMaskA = true;

bool GlStateManager::s_polygonOffsetFill    = false;
float GlStateManager::s_polygonOffsetFactor = 0.0f;
float GlStateManager::s_polygonOffsetUnits  = 0.0f;
ePOLYGON_MODE GlStateManager::s_polygonMode = ePolygonMode_Fill;

float GlStateManager::s_lineWidth = 1.0f;

glm::dmat4 &GlStateManager::GetActiveMatrix()
{
    if (s_matrixMode == 1)
    {
        return s_projectionMatrix;
    }

    return s_modelViewMatrix;
}

std::vector<glm::dmat4> &GlStateManager::GetActiveStack()
{
    if (s_matrixMode == 1)
    {
        return s_projectionStack;
    }

    return s_modelViewStack;
}

void GlStateManager::SetClearColor(float r, float g, float b, float a)
{
    RenderDevice::GetInstance()->SetClearColor(r, g, b, a);
}

void GlStateManager::ClearDepth()
{
    RenderDevice::GetInstance()->ClearDepth();
}

void GlStateManager::ClearAll()
{
    RenderDevice::GetInstance()->ClearAll();
}

void GlStateManager::EnableDepthTest()
{
    if (s_depthTest)
    {
        return;
    }

    s_depthTest = true;
    RenderDevice::GetInstance()->SetDepthTest(true);
}

void GlStateManager::DisableDepthTest()
{
    if (!s_depthTest)
    {
        return;
    }

    s_depthTest = false;
    RenderDevice::GetInstance()->SetDepthTest(false);
}

void GlStateManager::SetDepthFunc(eCOMPARE_FUNC func)
{
    if (s_depthFunc == func)
    {
        return;
    }

    s_depthFunc = func;
    RenderDevice::GetInstance()->SetDepthFunc(func);
}

void GlStateManager::SetDepthMask(bool enabled)
{
    if (s_depthMask == enabled)
    {
        return;
    }

    s_depthMask = enabled;
    RenderDevice::GetInstance()->SetDepthMask(enabled);
}

bool GlStateManager::IsDepthTestEnabled()
{
    return s_depthTest;
}

bool GlStateManager::IsDepthMaskEnabled()
{
    return s_depthMask;
}

void GlStateManager::EnableCull()
{
    if (s_cull)
    {
        return;
    }

    s_cull = true;
    RenderDevice::GetInstance()->SetCull(true);
}

void GlStateManager::DisableCull()
{
    if (!s_cull)
    {
        return;
    }

    s_cull = false;
    RenderDevice::GetInstance()->SetCull(false);
}

void GlStateManager::SetCullFace(eCULL_FACE face)
{
    if (s_cullFace == face)
    {
        return;
    }

    s_cullFace = face;
    RenderDevice::GetInstance()->SetCullFace(face);
}

void GlStateManager::SetFrontFace(eFRONT_FACE winding)
{
    if (s_frontFace == winding)
    {
        return;
    }

    s_frontFace = winding;
    RenderDevice::GetInstance()->SetFrontFace(winding);
}

void GlStateManager::EnableBlend()
{
    if (s_blend)
    {
        return;
    }

    s_blend = true;
    RenderDevice::GetInstance()->SetBlend(true);
}

void GlStateManager::DisableBlend()
{
    if (!s_blend)
    {
        return;
    }

    s_blend = false;
    RenderDevice::GetInstance()->SetBlend(false);
}

void GlStateManager::SetBlendFunc(eBLEND_FACTOR src, eBLEND_FACTOR dst)
{
    s_blendSrc = src;
    s_blendDst = dst;
    RenderDevice::GetInstance()->SetBlendFunc(src, dst);
}

void GlStateManager::EnableScissorTest()
{
    if (s_scissor)
    {
        return;
    }

    s_scissor = true;
    RenderDevice::GetInstance()->SetScissorTest(true);
}

void GlStateManager::DisableScissorTest()
{
    if (!s_scissor)
    {
        return;
    }

    s_scissor = false;
    RenderDevice::GetInstance()->SetScissorTest(false);
}

void GlStateManager::SetScissor(int x, int y, int w, int h)
{
    RenderDevice::GetInstance()->SetScissor(x, y, w, h);
}

void GlStateManager::EnableTexture()
{
    s_textureEnabled = true;
}

void GlStateManager::DisableTexture()
{
    s_textureEnabled = false;
}

bool GlStateManager::IsCullEnabled()
{
    return s_cull;
}

bool GlStateManager::IsBlendEnabled()
{
    return s_blend;
}

void GlStateManager::GetBlendFunc(eBLEND_FACTOR *src, eBLEND_FACTOR *dst)
{
    *src = s_blendSrc;
    *dst = s_blendDst;
}

bool GlStateManager::IsColorWriteEnabled()
{
    return s_colorMaskR;
}

bool GlStateManager::IsPolygonOffsetFillEnabled()
{
    return s_polygonOffsetFill;
}

void GlStateManager::GetPolygonOffset(float *factor, float *units)
{
    *factor = s_polygonOffsetFactor;
    *units  = s_polygonOffsetUnits;
}

bool GlStateManager::IsTextureEnabled()
{
    return s_textureEnabled;
}

void GlStateManager::SetAlphaCutoff(float cutoff)
{
    s_alphaCutoff = cutoff;
}

float GlStateManager::GetAlphaCutoff()
{
    return s_alphaCutoff;
}

void GlStateManager::EnableFog()
{
    s_fog = true;
}

void GlStateManager::DisableFog()
{
    s_fog = false;
}

bool GlStateManager::IsFogEnabled()
{
    return s_fog;
}

void GlStateManager::SetFogColor(float r, float g, float b)
{
    s_fogR = r;
    s_fogG = g;
    s_fogB = b;
}

void GlStateManager::GetFogColor(float *r, float *g, float *b)
{
    *r = s_fogR;
    *g = s_fogG;
    *b = s_fogB;
}

void GlStateManager::SetFogRange(float start, float end)
{
    s_fogStart = start;
    s_fogEnd   = end;
}

void GlStateManager::GetFogRange(float *start, float *end)
{
    *start = s_fogStart;
    *end   = s_fogEnd;
}

void GlStateManager::SetOverlayColor(uint32_t color)
{
    s_overlayColor = color;
}

uint32_t GlStateManager::GetOverlayColor()
{
    return s_overlayColor;
}

void GlStateManager::SetGlint(float strength)
{
    s_glint = strength;
}

float GlStateManager::GetGlint()
{
    return s_glint;
}

void GlStateManager::SetColorMask(bool r, bool g, bool b, bool a)
{
    if (s_colorMaskR == r && s_colorMaskG == g && s_colorMaskB == b && s_colorMaskA == a)
    {
        return;
    }

    s_colorMaskR = r;
    s_colorMaskG = g;
    s_colorMaskB = b;
    s_colorMaskA = a;
    RenderDevice::GetInstance()->SetColorMask(r, g, b, a);
}

void GlStateManager::EnablePolygonOffsetFill()
{
    if (s_polygonOffsetFill)
    {
        return;
    }

    s_polygonOffsetFill = true;
    RenderDevice::GetInstance()->SetPolygonOffsetFill(true);
}

void GlStateManager::DisablePolygonOffsetFill()
{
    if (!s_polygonOffsetFill)
    {
        return;
    }

    s_polygonOffsetFill = false;
    RenderDevice::GetInstance()->SetPolygonOffsetFill(false);
}

void GlStateManager::SetPolygonOffset(float factor, float units)
{
    if (s_polygonOffsetFactor == factor && s_polygonOffsetUnits == units)
    {
        return;
    }

    s_polygonOffsetFactor = factor;
    s_polygonOffsetUnits  = units;
    RenderDevice::GetInstance()->SetPolygonOffset(factor, units);
}

void GlStateManager::SetPolygonMode(ePOLYGON_MODE mode)
{
    if (s_polygonMode == mode)
    {
        return;
    }

    s_polygonMode = mode;
    RenderDevice::GetInstance()->SetPolygonMode(mode);
}

void GlStateManager::SetLineWidth(float width)
{
    if (s_lineWidth == width)
    {
        return;
    }

    s_lineWidth = width;
    RenderDevice::GetInstance()->SetLineWidth(width);
}

void GlStateManager::MatrixMode(int mode)
{
    if (mode != 0 && mode != 1)
    {
        return;
    }

    s_matrixMode = mode;
}

int GlStateManager::GetMatrixMode()
{
    return s_matrixMode;
}

void GlStateManager::PushMatrix()
{
    GetActiveStack().push_back(GetActiveMatrix());
}

void GlStateManager::PopMatrix()
{
    std::vector<glm::dmat4> &stack = GetActiveStack();
    if (stack.empty())
    {
        return;
    }

    GetActiveMatrix() = stack.back();
    stack.pop_back();
}

void GlStateManager::LoadIdentity()
{
    GetActiveMatrix() = glm::dmat4(1.0);
}

void GlStateManager::MultMatrix(const glm::dmat4 &mat)
{
    GetActiveMatrix() = GetActiveMatrix() * mat;
}

void GlStateManager::Ortho(float left, float right, float bottom, float top, float zNear, float zFar)
{
    GetActiveMatrix() = GetActiveMatrix() * glm::ortho((double) left, (double) right, (double) bottom, (double) top, (double) zNear, (double) zFar);
}

void GlStateManager::Perspective(float fovDegrees, float aspect, float zNear, float zFar)
{
    GetActiveMatrix() = GetActiveMatrix() * glm::perspective(glm::radians((double) fovDegrees), (double) aspect, (double) zNear, (double) zFar);
}

void GlStateManager::Translatef(float x, float y, float z)
{
    GetActiveMatrix() = glm::translate(GetActiveMatrix(), glm::dvec3(x, y, z));
}

void GlStateManager::Translated(double x, double y, double z)
{
    GetActiveMatrix() = glm::translate(GetActiveMatrix(), glm::dvec3(x, y, z));
}

void GlStateManager::Scalef(float x, float y, float z)
{
    GetActiveMatrix() = glm::scale(GetActiveMatrix(), glm::dvec3(x, y, z));
}

void GlStateManager::Rotatef(float angleDegrees, float x, float y, float z)
{
    GetActiveMatrix() = glm::rotate(GetActiveMatrix(), glm::radians((double) angleDegrees), glm::dvec3(x, y, z));
}

const glm::dmat4 &GlStateManager::GetMatrix()
{
    if (s_matrixMode == 1)
    {
        return s_projectionMatrix;
    }

    return s_modelViewMatrix;
}

#endif
