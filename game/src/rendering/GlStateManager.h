#pragma once

#include <cstdint>
#include <vector>

#include "RenderTypes.h"

#include <glm/mat4x4.hpp>

class GlStateManager
{
public:
    static void SetClearColor(float r, float g, float b, float a);
    static void ClearDepth();
    static void ClearAll();

    static void EnableDepthTest();
    static void DisableDepthTest();
    static void SetDepthFunc(eCOMPARE_FUNC func);
    static void SetDepthMask(bool enabled);
    static bool IsDepthTestEnabled();
    static bool IsDepthMaskEnabled();

    static void EnableCull();
    static void DisableCull();
    static bool IsCullEnabled();
    static void SetCullFace(eCULL_FACE face);
    static void SetFrontFace(eFRONT_FACE winding);

    static void EnableBlend();
    static void DisableBlend();
    static void SetBlendFunc(eBLEND_FACTOR src, eBLEND_FACTOR dst);
    static bool IsBlendEnabled();
    static void GetBlendFunc(eBLEND_FACTOR *src, eBLEND_FACTOR *dst);

    static void EnableScissorTest();
    static void DisableScissorTest();
    static void SetScissor(int x, int y, int w, int h);

    static void EnableTexture();
    static void DisableTexture();
    static bool IsTextureEnabled();

    static void SetAlphaCutoff(float cutoff);
    static float GetAlphaCutoff();

    static void EnableFog();
    static void DisableFog();
    static bool IsFogEnabled();

    static void SetFogColor(float r, float g, float b);
    static void GetFogColor(float *r, float *g, float *b);

    static void SetFogRange(float start, float end);
    static void GetFogRange(float *start, float *end);

    static void SetOverlayColor(uint32_t color);
    static uint32_t GetOverlayColor();

    static void SetGlint(float strength);
    static float GetGlint();

    static void SetColorMask(bool r, bool g, bool b, bool a);
    static bool IsColorWriteEnabled();

    static void EnablePolygonOffsetFill();
    static void DisablePolygonOffsetFill();
    static void SetPolygonOffset(float factor, float units);
    static bool IsPolygonOffsetFillEnabled();
    static void GetPolygonOffset(float *factor, float *units);
    static void SetPolygonMode(ePOLYGON_MODE mode);

    static void SetLineWidth(float width);

    static void MatrixMode(int mode);
    static int GetMatrixMode();

    static void PushMatrix();
    static void PopMatrix();
    static void LoadIdentity();
    static void MultMatrix(const glm::dmat4 &mat);

    static void Ortho(float left, float right, float bottom, float top, float zNear, float zFar);
    static void Perspective(float fovDegrees, float aspect, float zNear, float zFar);

    static void Translatef(float x, float y, float z);
    static void Translated(double x, double y, double z);
    static void Scalef(float x, float y, float z);
    static void Rotatef(float angleDegrees, float x, float y, float z);

    static const glm::dmat4 &GetMatrix();

private:
    static glm::dmat4 &GetActiveMatrix();
    static std::vector<glm::dmat4> &GetActiveStack();

    static std::vector<glm::dmat4> s_modelViewStack;
    static std::vector<glm::dmat4> s_projectionStack;
    static glm::dmat4 s_modelViewMatrix;
    static glm::dmat4 s_projectionMatrix;
    static int s_matrixMode;

    static bool s_depthTest;
    static bool s_depthMask;
    static eCOMPARE_FUNC s_depthFunc;

    static bool s_cull;
    static eCULL_FACE s_cullFace;
    static eFRONT_FACE s_frontFace;

    static bool s_blend;
    static eBLEND_FACTOR s_blendSrc;
    static eBLEND_FACTOR s_blendDst;

    static bool s_scissor;
    static bool s_textureEnabled;
    static float s_alphaCutoff;

    static bool s_fog;
    static float s_fogR;
    static float s_fogG;
    static float s_fogB;
    static float s_fogStart;
    static float s_fogEnd;
    static uint32_t s_overlayColor;
    static float s_glint;

    static bool s_colorMaskR;
    static bool s_colorMaskG;
    static bool s_colorMaskB;
    static bool s_colorMaskA;

    static bool s_polygonOffsetFill;
    static float s_polygonOffsetFactor;
    static float s_polygonOffsetUnits;
    static ePOLYGON_MODE s_polygonMode;

    static float s_lineWidth;
};
