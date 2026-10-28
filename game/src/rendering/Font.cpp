#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Font.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <stdexcept>

#include "../core/Everstead.h"
#include "../ui/Screen.h"
#include "../world/block/BlockRenderer.h"
#include "BufferedImage.h"
#include "GlStateManager.h"
#include "RenderDevice.h"
#include "Tesselator.h"
#include "Textures.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <nlohmann/json.hpp>

Font::Font(const char *fontPath, int pixelHeight)
{
    m_shader.Load("assets/shader/font.vert", "assets/shader/font.frag");
    m_atlas             = nullptr;
    m_vao               = {};
    m_vbo               = {};
    m_nearest           = true;
    m_shadowOffsetX     = 1.0f;
    m_shadowOffsetY     = 1.0f;
    m_shadowOffsetZ     = 0.001f;
    m_ascent            = (float) std::max(pixelHeight - 1, 1);
    m_lineHeight        = (float) std::max(pixelHeight, 1);
    m_bitmapScale       = (float) std::max(pixelHeight, 1) / 8.0f;
    m_cellPixelWidth    = 8;
    m_cellPixelHeight   = 8;
    m_glyphColumns      = 0;
    m_glyphRows         = 0;
    m_fallbackCodepoint = (uint32_t) '?';

    if (IsTrueTypePath(fontPath))
    {
        LoadTrueTypeGlyphs(fontPath, pixelHeight);
    }
    else
    {
        BufferedImage image(fontPath);
        m_glyphColumns = image.GetPixelWidth() / m_cellPixelWidth;
        m_glyphRows    = image.GetPixelHeight() / m_cellPixelHeight;
        if (m_glyphColumns <= 0 || m_glyphRows <= 0)
        {
            throw std::runtime_error("invalid bitmap font atlas size");
        }

        m_ascent     = (float) (m_cellPixelHeight - 1) * m_bitmapScale;
        m_lineHeight = (float) m_cellPixelHeight * m_bitmapScale;
        LoadBitmapGlyphs(image);
    }

    if (!m_atlas)
    {
        throw std::runtime_error("failed to load font");
    }

    RenderDevice *device = RenderDevice::GetInstance();
    m_vao                = device->CreateVertexArray();
    m_vbo                = device->CreateBuffer();

    device->BindVertexArray(m_vao);
    device->BindArrayBuffer(m_vbo);
    device->SetVertexAttrib(0, 3, eVertexAttribType_Float, false, s_vertexFloats * sizeof(float), 0);
    device->SetVertexAttrib(1, 2, eVertexAttribType_Float, false, s_vertexFloats * sizeof(float), 3 * sizeof(float));
    device->SetVertexAttrib(2, 4, eVertexAttribType_Float, false, s_vertexFloats * sizeof(float), 5 * sizeof(float));

    m_shader.Bind();
    SetNearest(m_nearest);
}

Font::~Font()
{
    RenderDevice::GetInstance()->DeleteBuffer(m_vbo);
    RenderDevice::GetInstance()->DeleteVertexArray(m_vao);
}

void Font::SetNearest(bool nearest)
{
    m_nearest = nearest;

    if (!m_atlas)
    {
        return;
    }

    eTEXTURE_FILTER filter = m_nearest ? eTextureFilter_Nearest : eTextureFilter_Linear;
    m_atlas->SetFilterMode(filter, filter);
    m_atlas->SetWrapMode(eTextureWrap_ClampToEdge, eTextureWrap_ClampToEdge);
}

void Font::SetShadowOffset(float x, float y, float z)
{
    m_shadowOffsetX = x;
    m_shadowOffsetY = y;
    m_shadowOffsetZ = z;
}

float Font::GetWidth(std::wstring_view text, float scale) const
{
    if (text.empty())
    {
        return 0.0f;
    }

    float width          = 0.0f;
    float effectiveScale = SnapScale(scale);

    for (wchar_t wc : text)
    {
        if (wc == L'\n')
        {
            break;
        }

        uint32_t cp        = (uint32_t) wc;
        const Glyph &glyph = GetGlyph(cp);
        if (glyph.advance <= 0.0f)
        {
            continue;
        }

        width += glyph.advance * m_bitmapScale * effectiveScale;
    }

    return width;
}

float Font::GetAscent(float scale) const
{
    return m_ascent * SnapScale(scale);
}

float Font::GetLineHeight(float scale) const
{
    return m_lineHeight * SnapScale(scale);
}

float Font::GetPixelSize(float scale) const
{
    return (m_lineHeight - m_ascent) * SnapScale(scale);
}

float Font::GetGlyphPixelSize(float scale) const
{
    float pixelSize = scale * m_bitmapScale;
    if (pixelSize < kMinPixelPerfectSize || fabsf(pixelSize - roundf(pixelSize)) < 0.001f)
    {
        return pixelSize;
    }

    Everstead *everstead = Everstead::GetInstance();
    float ratio          = Screen::ScaleUniform(everstead->GetWidth(), everstead->GetHeight()) /
                           Screen::ScaleUniform(Screen::REFERENCE_WIDTH, Screen::REFERENCE_HEIGHT);
    return std::max(1.0f, roundf(pixelSize / ratio)) * ratio;
}

float Font::GetShadowPixelOffset(float offset, float scale) const
{
    return std::max(1.0f, roundf(offset * GetGlyphPixelSize(scale)));
}

float Font::GetPixelPerfectScale(float pixelSize) const
{
    return std::max(1.0f, roundf(pixelSize)) / m_bitmapScale;
}

float Font::SnapScale(float scale) const
{
    return GetGlyphPixelSize(scale) / m_bitmapScale;
}

void Font::Draw(std::wstring_view text, float x, float y, float scale, uint32_t argb)
{
    AppendText2D(text, x, y, scale, argb, false);
    Flush2D();
}

void Font::DrawShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb)
{
    DrawShadowStrength(text, x, y, scale, argb, 0.25f);
}

void Font::DrawShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb, uint32_t shadowColor)
{
    AppendText2D(text, x + GetShadowPixelOffset(m_shadowOffsetX, scale), y + GetShadowPixelOffset(m_shadowOffsetY, scale), scale, shadowColor, false);
    AppendText2D(text, x, y, scale, argb, false);
    Flush2D();
}

void Font::QueueShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb)
{
    uint32_t shadowColor = MultiplyColor(argb, 0.25f);
    AppendText2D(text, x + GetShadowPixelOffset(m_shadowOffsetX, scale), y + GetShadowPixelOffset(m_shadowOffsetY, scale), scale, shadowColor, false);
    AppendText2D(text, x, y, scale, argb, false);
}

void Font::Flush()
{
    Flush2D();
}

void Font::DrawShadowStrength(std::wstring_view text, float x, float y, float scale, uint32_t argb, float shadowStrength)
{
    DrawShadow(text, x, y, scale, argb, MultiplyColor(argb, shadowStrength));
}

void Font::DrawStyled(std::wstring_view text, float x, float y, float scale, uint32_t argb, bool italic)
{
    AppendText2D(text, x, y, scale, argb, italic);
    Flush2D();
}

void Font::DrawStyledShadow(std::wstring_view text, float x, float y, float scale, uint32_t argb, bool italic)
{
    uint32_t shadowColor = MultiplyColor(argb, 0.25f);
    AppendText2D(text, x + GetShadowPixelOffset(m_shadowOffsetX, scale), y + GetShadowPixelOffset(m_shadowOffsetY, scale), scale, shadowColor,
                 italic);
    AppendText2D(text, x, y, scale, argb, italic);
    Flush2D();
}

void Font::Draw(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb)
{
    AppendText3D(text, x + BlockRenderer::xOff, y + BlockRenderer::yOff, z + BlockRenderer::zOff, scale, argb);
    Flush3D();
}

void Font::DrawShadow(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb)
{
    DrawShadow(text, x, y, z, scale, argb, MultiplyColor(argb, 0.25f));
}

void Font::DrawShadow(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb, uint32_t shadowColor)
{
    float ox = m_shadowOffsetX * scale * m_bitmapScale;
    float oy = m_shadowOffsetY * scale * m_bitmapScale;
    float oz = m_shadowOffsetZ;
    if (oz == 0.0f)
    {
        oz = 0.01f;
    }

    oz *= scale;

    float bx = BlockRenderer::xOff;
    float by = BlockRenderer::yOff;
    float bz = BlockRenderer::zOff;
    AppendText3D(text, x + ox + bx, y - oy + by, z - oz + bz, scale, shadowColor);
    AppendText3D(text, x + bx, y + by, z + bz, scale, argb);
    Flush3D();
}

float Font::SnapBitmapPixel(float value) const
{
    return m_nearest ? round(value) : value;
}

uint32_t Font::MultiplyColor(uint32_t argb, float mul) const
{
    uint32_t a = (argb >> 24) & 0xFF;
    uint32_t r = (argb >> 16) & 0xFF;
    uint32_t g = (argb >> 8) & 0xFF;
    uint32_t b = argb & 0xFF;

    r = (uint32_t) (r * mul);
    g = (uint32_t) (g * mul);
    b = (uint32_t) (b * mul);

    if (r > 255)
    {
        r = 255;
    }

    if (g > 255)
    {
        g = 255;
    }

    if (b > 255)
    {
        b = 255;
    }

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void Font::ColorToFloats(uint32_t argb, float *r, float *g, float *b, float *a) const
{
    *a = ((argb >> 24) & 0xFF) / 255.0f;
    *r = ((argb >> 16) & 0xFF) / 255.0f;
    *g = ((argb >> 8) & 0xFF) / 255.0f;
    *b = (argb & 0xFF) / 255.0f;
}

bool Font::AtlasUsesAlpha(const std::vector<uint8_t> &pixels) const
{
    if (pixels.empty())
    {
        return true;
    }

    for (size_t i = 3; i < pixels.size(); i += 4)
    {
        uint8_t a = pixels[i];
        if (a != 255)
        {
            return true;
        }
    }

    return false;
}

bool Font::IsTrueTypePath(const char *fontPath) const
{
    if (!fontPath)
    {
        return false;
    }

    std::string path(fontPath);
    for (size_t i = 0; i < path.size(); i++)
    {
        path[i] = (char) std::tolower((unsigned char) path[i]);
    }

    return path.size() >= 4 && path.substr(path.size() - 4) == ".ttf";
}

void Font::LoadTrueTypeGlyphs(const char *fontPath, int pixelHeight)
{
    const uint32_t firstGlyph = 32;
    const uint32_t lastGlyph  = 126;
    const int padding         = 2;
    const int atlasColumns    = 16;

    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) != 0)
    {
        throw std::runtime_error("failed to initialize FreeType");
    }

    FT_Face face = nullptr;
    if (FT_New_Face(library, fontPath, 0, &face) != 0)
    {
        FT_Done_FreeType(library);
        throw std::runtime_error(std::string("failed to load font: ") + fontPath);
    }

    int clampedPixelHeight = std::max(pixelHeight, 1);
    FT_Set_Pixel_Sizes(face, 0, (FT_UInt) clampedPixelHeight);

    int maxGlyphWidth  = 1;
    int maxGlyphHeight = 1;
    int glyphCount     = (int) (lastGlyph - firstGlyph + 1);
    for (uint32_t cp = firstGlyph; cp <= lastGlyph; cp++)
    {
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER) != 0)
        {
            continue;
        }

        maxGlyphWidth  = std::max(maxGlyphWidth, (int) face->glyph->bitmap.width);
        maxGlyphHeight = std::max(maxGlyphHeight, (int) face->glyph->bitmap.rows);
    }

    int cellWidth   = maxGlyphWidth + padding * 2;
    int cellHeight  = maxGlyphHeight + padding * 2;
    int atlasRows   = (glyphCount + atlasColumns - 1) / atlasColumns;
    int atlasWidth  = atlasColumns * cellWidth;
    int atlasHeight = atlasRows * cellHeight;

    std::vector<uint8_t> pixels((size_t) atlasWidth * (size_t) atlasHeight * 4, 0);
    m_glyphs.assign((size_t) lastGlyph + 1, Glyph{});
    m_bitmapScale       = 1.0f;
    m_cellPixelWidth    = cellWidth;
    m_cellPixelHeight   = cellHeight;
    m_glyphColumns      = atlasColumns;
    m_glyphRows         = atlasRows;
    m_fallbackCodepoint = (uint32_t) '?';

    float ascent     = (float) (face->size->metrics.ascender >> 6);
    float lineHeight = (float) (face->size->metrics.height >> 6);
    m_ascent         = ascent > 0.0f ? ascent : (float) clampedPixelHeight;
    m_lineHeight     = lineHeight > 0.0f ? lineHeight : (float) clampedPixelHeight * 1.2f;

    for (uint32_t cp = firstGlyph; cp <= lastGlyph; cp++)
    {
        int glyphIndex = (int) (cp - firstGlyph);
        int cellX      = (glyphIndex % atlasColumns) * cellWidth;
        int cellY      = (glyphIndex / atlasColumns) * cellHeight;
        int targetX    = cellX + padding;
        int targetY    = cellY + padding;

        Glyph glyph{};
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER) == 0)
        {
            FT_GlyphSlot slot = face->glyph;
            int bitmapWidth   = (int) slot->bitmap.width;
            int bitmapHeight  = (int) slot->bitmap.rows;

            for (int y = 0; y < bitmapHeight; y++)
            {
                for (int x = 0; x < bitmapWidth; x++)
                {
                    uint8_t coverage        = slot->bitmap.buffer[(size_t) y * slot->bitmap.pitch + x];
                    size_t targetIndex      = ((size_t) (targetY + y) * (size_t) atlasWidth + (size_t) (targetX + x)) * 4;
                    pixels[targetIndex + 0] = 255;
                    pixels[targetIndex + 1] = 255;
                    pixels[targetIndex + 2] = 255;
                    pixels[targetIndex + 3] = coverage;
                }
            }

            glyph.u0       = (float) targetX / (float) atlasWidth;
            glyph.v0       = (float) targetY / (float) atlasHeight;
            glyph.u1       = (float) (targetX + bitmapWidth) / (float) atlasWidth;
            glyph.v1       = (float) (targetY + bitmapHeight) / (float) atlasHeight;
            glyph.width    = (float) bitmapWidth;
            glyph.height   = (float) bitmapHeight;
            glyph.bearingX = (float) slot->bitmap_left;
            glyph.bearingY = (float) slot->bitmap_top;
            glyph.advance  = (float) (slot->advance.x >> 6);
        }

        m_glyphs[(size_t) cp] = glyph;
    }

    if ((size_t) ' ' < m_glyphs.size() && m_glyphs[(size_t) ' '].advance <= 0.0f)
    {
        m_glyphs[(size_t) ' '].advance = (float) clampedPixelHeight * 0.35f;
    }

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    BufferedImage image(atlasWidth, atlasHeight, pixels.data());
    m_atlas = std::make_unique<Texture>(image);
    SetNearest(false);
}

uint8_t Font::GetCoverage(const std::vector<uint8_t> &pixels, int width, int x, int y, bool useAlpha) const
{
    size_t index = ((size_t) y * (size_t) width + (size_t) x) * (size_t) 4;

    uint8_t r = pixels[index + 0];
    uint8_t g = pixels[index + 1];
    uint8_t b = pixels[index + 2];
    uint8_t a = pixels[index + 3];

    if (useAlpha)
    {
        return a;
    }

    uint8_t maxRgb = std::max(r, std::max(g, b));
    return (uint8_t) (255 - maxRgb);
}

void Font::WriteCoveragePixel(std::vector<uint8_t> *pixels, int width, int x, int y, uint8_t coverage) const
{
    if (!pixels)
    {
        return;
    }

    size_t index         = ((size_t) y * (size_t) width + (size_t) x) * (size_t) 4;
    (*pixels)[index + 0] = 255;
    (*pixels)[index + 1] = 255;
    (*pixels)[index + 2] = 255;
    (*pixels)[index + 3] = coverage;
}

void Font::CopyGlyphPixel(const std::vector<uint8_t> &sourcePixels, int sourceWidth, int sourceX, int sourceY, std::vector<uint8_t> *targetPixels,
                          int targetWidth, int targetX, int targetY, bool useAlpha) const
{
    uint8_t coverage = GetCoverage(sourcePixels, sourceWidth, sourceX, sourceY, useAlpha);
    WriteCoveragePixel(targetPixels, targetWidth, targetX, targetY, coverage);
}

void Font::AppendQuad(float x0, float y0, float x1, float y1, float z, float shear0, float shear1, float u0, float v0, float u1, float v1,
                      uint32_t argb)
{
    float r;
    float g;
    float b;
    float a;
    ColorToFloats(argb, &r, &g, &b, &a);

    float verts[] = {
            x0 + shear0, y0, z, u0, v0, r, g, b, a, x1 + shear0, y0, z, u1, v0, r, g, b, a, x0 + shear1, y1, z, u0, v1, r, g, b, a,
            x1 + shear0, y0, z, u1, v0, r, g, b, a, x1 + shear1, y1, z, u1, v1, r, g, b, a, x0 + shear1, y1, z, u0, v1, r, g, b, a,
    };
    m_batch.insert(m_batch.end(), verts, verts + 6 * s_vertexFloats);
}

void Font::AppendText2D(std::wstring_view text, float x, float y, float scale, uint32_t argb, bool italic)
{
    float glyphScale     = GetGlyphPixelSize(scale);
    float effectiveScale = SnapScale(scale);

    float originX = SnapBitmapPixel(x);
    float penX    = originX;
    float penY    = SnapBitmapPixel(y);

    for (wchar_t wc : text)
    {
        uint32_t cp = (uint32_t) wc;
        if (cp == (uint32_t) L'\n')
        {
            penX = originX;
            penY += SnapBitmapPixel(m_lineHeight * effectiveScale);
            continue;
        }

        const Glyph &ch = GetGlyph(cp);
        if (ch.advance <= 0.0f)
        {
            continue;
        }

        if (ch.width > 0.0f && ch.height > 0.0f)
        {
            float left   = penX;
            float top    = penY - ch.bearingY * glyphScale;
            float right  = left + ch.width * glyphScale;
            float bottom = top + ch.height * glyphScale;
            float shear  = italic ? ch.height * glyphScale * 0.25f : 0.0f;
            AppendQuad(left, top, right, bottom, 0.0f, shear, 0.0f, ch.u0, ch.v0, ch.u1, ch.v1, argb);
        }

        penX += ch.advance * glyphScale;
    }
}

void Font::Flush2D()
{
    if (m_batch.empty())
    {
        return;
    }

    GlStateManager::DisableDepthTest();
    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    SubmitBatch();

    GlStateManager::SetDepthMask(true);
    GlStateManager::EnableDepthTest();
}

void Font::AppendText3D(std::wstring_view text, float x, float y, float z, float scale, uint32_t argb)
{
    float glyphScale = scale * m_bitmapScale;
    float penX       = 0.0f;
    float penY       = 0.0f;

    for (wchar_t wc : text)
    {
        uint32_t cp = (uint32_t) wc;
        if (cp == (uint32_t) L'\n')
        {
            penX = 0.0f;
            penY -= m_lineHeight * scale;
            continue;
        }

        const Glyph &ch = GetGlyph(cp);
        if (ch.advance <= 0.0f)
        {
            continue;
        }

        if (ch.width > 0.0f && ch.height > 0.0f)
        {
            float x0 = x + penX;
            float y0 = y + penY + ch.bearingY * glyphScale;
            float x1 = x0 + ch.width * glyphScale;
            float y1 = y0 - ch.height * glyphScale;
            AppendQuad(x0, y0, x1, y1, z, 0.0f, 0.0f, ch.u0, ch.v0, ch.u1, ch.v1, argb);
        }

        penX += ch.advance * glyphScale;
    }
}

void Font::Flush3D()
{
    if (m_batch.empty())
    {
        return;
    }

    GlStateManager::SetDepthMask(false);
    GlStateManager::DisableCull();
    GlStateManager::EnableBlend();
    GlStateManager::SetBlendFunc(eBlendFactor_SrcAlpha, eBlendFactor_OneMinusSrcAlpha);

    SubmitBatch();

    GlStateManager::SetDepthMask(true);
}

void Font::SubmitBatch()
{
    m_shader.Bind();

    GlStateManager::MatrixMode(0);
    m_shader.SetMat4("u_view", glm::value_ptr(GlStateManager::GetMatrix()));

    GlStateManager::MatrixMode(1);
    m_shader.SetMat4("u_projection", glm::value_ptr(GlStateManager::GetMatrix()));

    GlStateManager::MatrixMode(0);
    m_shader.SetMat4("u_model", glm::value_ptr(glm::dmat4(1.0)));

    RenderDevice *device = RenderDevice::GetInstance();
    device->BindVertexArray(m_vao);
    device->BindArrayBuffer(m_vbo);
    if (m_atlas)
    {
        Textures::Bind(m_atlas->GetId(), 0);
    }

    device->UploadArrayBuffer(m_batch.data(), (uint32_t) (m_batch.size() * sizeof(float)), eBufferUsage_Stream);
    device->DrawArrays(ePrimitiveType_Triangles, 0, (int) (m_batch.size() / s_vertexFloats));

    m_batch.clear();
}

const Font::Glyph &Font::GetGlyph(uint32_t codepoint) const
{
    if (codepoint < m_glyphs.size())
    {
        return m_glyphs[codepoint];
    }

    std::unordered_map<uint32_t, Glyph>::const_iterator extra = m_extraGlyphs.find(codepoint);
    if (extra != m_extraGlyphs.end())
    {
        return extra->second;
    }

    if (m_fallbackCodepoint < m_glyphs.size())
    {
        return m_glyphs[m_fallbackCodepoint];
    }

    static const Glyph empty{};
    return empty;
}

void Font::CollectBitmapGlyphs(const BufferedImage &image, const std::vector<std::vector<uint32_t>> &chars, int height, int ascent, bool primary,
                               std::vector<PendingGlyph> *pending)
{
    const std::vector<uint8_t> &pixels = image.GetPixels();
    bool useAlpha                      = AtlasUsesAlpha(pixels);
    int cellWidth                      = image.GetPixelWidth() / (int) chars[0].size();
    int cellHeight                     = image.GetPixelHeight() / (int) chars.size();
    float scale                        = (float) height / (float) cellHeight;

    for (size_t row = 0; row < chars.size(); row++)
    {
        for (size_t column = 0; column < chars[row].size(); column++)
        {
            uint32_t codepoint = chars[row][column];
            // Later providers use 0 for empty cells and only fill codepoints nothing before them covered
            if (!primary && (codepoint == 0 || codepoint < m_glyphs.size() || m_extraGlyphs.contains(codepoint)))
            {
                continue;
            }

            int cellX0     = (int) column * cellWidth;
            int cellY0     = (int) row * cellHeight;
            int glyphWidth = 0;
            for (int px = cellWidth - 1; px >= 0 && glyphWidth == 0; px--)
            {
                for (int py = 0; py < cellHeight; py++)
                {
                    if (GetCoverage(pixels, image.GetPixelWidth(), cellX0 + px, cellY0 + py, useAlpha) != 0)
                    {
                        glyphWidth = px + 1;
                        break;
                    }
                }
            }

            Glyph &glyph = codepoint < m_glyphs.size() ? m_glyphs[codepoint] : m_extraGlyphs[codepoint];
            if (glyphWidth == 0)
            {
                glyph.advance = codepoint == (uint32_t) ' ' ? 4.0f : 0.0f;
                continue;
            }

            glyph.width    = (float) glyphWidth * scale;
            glyph.height   = (float) height;
            glyph.bearingY = (float) ascent;
            glyph.advance  = primary ? (float) std::min(glyphWidth + 1, cellWidth) : glyph.width + 1.0f;
            pending->push_back({&image, codepoint, cellX0, cellY0, glyphWidth, cellHeight, 0, 0, useAlpha});
        }
    }
}

void Font::LoadBitmapGlyphs(const BufferedImage &image)
{
    m_glyphs.assign((size_t) m_glyphColumns * (size_t) m_glyphRows, Glyph{});

    std::vector<std::vector<uint32_t>> defaultChars((size_t) m_glyphRows, std::vector<uint32_t>((size_t) m_glyphColumns));
    for (int row = 0; row < m_glyphRows; row++)
    {
        for (int column = 0; column < m_glyphColumns; column++)
        {
            defaultChars[(size_t) row][(size_t) column] = (uint32_t) (row * m_glyphColumns + column);
        }
    }

    std::vector<PendingGlyph> pending;
    CollectBitmapGlyphs(image, defaultChars, m_cellPixelHeight, m_cellPixelHeight - 1, true, &pending);

    std::ifstream stream("assets/font/providers.json");
    nlohmann::json providers = nlohmann::json::parse(stream)["providers"];
    std::vector<BufferedImage> providerImages(providers.size());
    for (size_t i = 0; i < providers.size(); i++)
    {
        providerImages[i].Load(providers[i]["file"].get<std::string>().c_str());
        CollectBitmapGlyphs(providerImages[i], providers[i]["chars"].get<std::vector<std::vector<uint32_t>>>(), providers[i]["height"].get<int>(),
                            providers[i]["ascent"].get<int>(), false, &pending);
    }

    int atlasWidth = 256;
    int penX       = 0;
    int penY       = 0;
    int rowHeight  = 0;
    for (size_t i = 0; i < pending.size(); i++)
    {
        if (penX + pending[i].width + 2 > atlasWidth)
        {
            penX = 0;
            penY += rowHeight;
            rowHeight = 0;
        }

        pending[i].atlasX = penX + 1;
        pending[i].atlasY = penY + 1;
        penX += pending[i].width + 2;
        rowHeight = std::max(rowHeight, pending[i].height + 2);
    }

    int atlasHeight = penY + rowHeight;
    std::vector<uint8_t> atlasPixels((size_t) atlasWidth * (size_t) atlasHeight * (size_t) 4, 0);
    for (size_t i = 0; i < pending.size(); i++)
    {
        const PendingGlyph &entry          = pending[i];
        const std::vector<uint8_t> &source = entry.image->GetPixels();
        int sourceWidth                    = entry.image->GetPixelWidth();

        // We copy one pixel past every edge so linear filtering never bleeds in a neighboring glyph
        for (int py = -1; py <= entry.height; py++)
        {
            for (int px = -1; px <= entry.width; px++)
            {
                int sourceX = entry.sourceX + std::clamp(px, 0, entry.width - 1);
                int sourceY = entry.sourceY + std::clamp(py, 0, entry.height - 1);
                CopyGlyphPixel(source, sourceWidth, sourceX, sourceY, &atlasPixels, atlasWidth, entry.atlasX + px, entry.atlasY + py, entry.useAlpha);
            }
        }

        Glyph &glyph = entry.codepoint < m_glyphs.size() ? m_glyphs[entry.codepoint] : m_extraGlyphs[entry.codepoint];
        glyph.u0     = (float) entry.atlasX / (float) atlasWidth;
        glyph.v0     = (float) entry.atlasY / (float) atlasHeight;
        glyph.u1     = (float) (entry.atlasX + entry.width) / (float) atlasWidth;
        glyph.v1     = (float) (entry.atlasY + entry.height) / (float) atlasHeight;
    }

    if ((size_t) ' ' < m_glyphs.size() && m_glyphs[(size_t) ' '].advance <= 0.0f)
    {
        m_glyphs[(size_t) ' '].advance = 4.0f;
    }

    BufferedImage atlas(atlasWidth, atlasHeight, atlasPixels.data());
    m_atlas = std::make_unique<Texture>(atlas);
    SetNearest(m_nearest);
}

#endif
