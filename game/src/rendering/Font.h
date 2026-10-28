#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "BufferedImage.h"
#include "RenderTypes.h"
#include "Shader.h"
#include "Texture.h"

class Font
{
public:
    Font(const char *fontPath, int pixelHeight);
    ~Font();

    Font(const Font &)            = delete;
    Font &operator=(const Font &) = delete;

    void SetNearest(bool nearest);
    void SetShadowOffset(float x, float y, float z);

    float GetWidth(std::wstring_view text, float scale) const;
    float GetAscent(float scale) const;
    float GetLineHeight(float scale) const;
    float GetPixelSize(float scale) const;
    float SnapScale(float scale) const;
    float GetShadowPixelOffset(float offset, float scale) const;
    float GetPixelPerfectScale(float pixelSize) const;

    void Draw(std::wstring_view text, float x, float y, float scale, uint32_t argb);
    void DrawShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb);
    void DrawShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb, uint32_t shadowColor);
    void DrawShadowStrength(std::wstring_view text, float x, float y, float scale, uint32_t argb, float shadowStrength);
    void DrawStyled(std::wstring_view text, float x, float y, float scale, uint32_t argb, bool italic);
    void DrawStyledShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb, bool italic);
    void QueueShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb);
    void Flush();

    void Draw(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb);
    void DrawShadow(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb);
    void DrawShadow(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb, uint32_t shadowColor);

private:
    static const int s_vertexFloats             = 9;
    static constexpr float kMinPixelPerfectSize = 0.5f;

    struct Glyph
    {
        float u0       = 0.0f;
        float v0       = 0.0f;
        float u1       = 0.0f;
        float v1       = 0.0f;
        float width    = 0.0f;
        float height   = 0.0f;
        float bearingX = 0.0f;
        float bearingY = 0.0f;
        float advance  = 0.0f;
    };

    struct PendingGlyph
    {
        const BufferedImage *image;
        uint32_t codepoint;
        int sourceX;
        int sourceY;
        int width;
        int height;
        int atlasX;
        int atlasY;
        bool useAlpha;
    };

    float SnapBitmapPixel(float value) const;
    uint32_t MultiplyColor(uint32_t argb, float mul) const;
    void ColorToFloats(uint32_t argb, float *r, float *g, float *b, float *a) const;
    bool AtlasUsesAlpha(const std::vector<uint8_t> &pixels) const;
    bool IsTrueTypePath(const char *fontPath) const;
    void LoadTrueTypeGlyphs(const char *fontPath, int pixelHeight);
    uint8_t GetCoverage(const std::vector<uint8_t> &pixels, int width, int x, int y, bool useAlpha) const;
    void WriteCoveragePixel(std::vector<uint8_t> *pixels, int width, int x, int y, uint8_t coverage) const;
    void CopyGlyphPixel(const std::vector<uint8_t> &sourcePixels, int sourceWidth, int sourceX, int sourceY, std::vector<uint8_t> *targetPixels,
                        int targetWidth, int targetX, int targetY, bool useAlpha) const;

    float GetGlyphPixelSize(float scale) const;
    void AppendQuad(float x0, float y0, float x1, float y1, float z, float shear0, float shear1, float u0, float v0, float u1, float v1,
                    uint32_t argb);
    void AppendText2D(std::wstring_view text, float x, float y, float scale, uint32_t argb, bool italic);
    void AppendText3D(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb);
    void Flush2D();
    void Flush3D();
    void SubmitBatch();
    const Glyph &GetGlyph(uint32_t codepoint) const;
    void CollectBitmapGlyphs(const BufferedImage &image, const std::vector<std::vector<uint32_t>> &chars, int height, int ascent, bool primary,
                             std::vector<PendingGlyph> *pending);
    void LoadBitmapGlyphs(const BufferedImage &image);

    Shader m_shader;
    std::unique_ptr<Texture> m_atlas;

    VertexArrayHandle m_vao;
    BufferHandle m_vbo;

    bool m_nearest;

    float m_shadowOffsetX;
    float m_shadowOffsetY;
    float m_shadowOffsetZ;
    float m_ascent;
    float m_lineHeight;
    float m_bitmapScale;
    int m_cellPixelWidth;
    int m_cellPixelHeight;
    int m_glyphColumns;
    int m_glyphRows;
    uint32_t m_fallbackCodepoint;

    std::vector<Glyph> m_glyphs;
    std::unordered_map<uint32_t, Glyph> m_extraGlyphs;
    std::vector<float> m_batch;
};
