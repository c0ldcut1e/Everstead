#pragma once

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace Screen
{
    const float WIDTH        = 1280.0f;
    const float HEIGHT       = 720.0f;
    const float SCALE_ADJUST = -0.2f;

    // We snap text to whole pixels at this size and scale it proportionally on other screens
    const int REFERENCE_WIDTH  = 2560;
    const int REFERENCE_HEIGHT = 1440;

    struct Rect
    {
        float x;
        float y;
        float width;
        float height;
    };

    inline glm::dmat4 BuildProjection()
    {
        return glm::ortho(0.0, (double) WIDTH, (double) HEIGHT, 0.0, -1.0, 1.0);
    }

    inline float ScaleX(int actualWidth)
    {
        return actualWidth > 0 ? (float) actualWidth / WIDTH : 1.0f;
    }

    inline float ScaleY(int actualHeight)
    {
        return actualHeight > 0 ? (float) actualHeight / HEIGHT : 1.0f;
    }

    inline float ScaleUniform(int actualWidth, int actualHeight)
    {
        return std::min(ScaleX(actualWidth), ScaleY(actualHeight)) * (1.0f + SCALE_ADJUST);
    }

    inline float ToActualX(float virtualX, int actualWidth, int actualHeight)
    {
        if (actualWidth <= 0 || actualHeight <= 0)
        {
            return virtualX;
        }

        float uniformBase        = std::min(ScaleX(actualWidth), ScaleY(actualHeight));
        float scaledVirtualWidth = WIDTH * uniformBase;
        float horizontalInset    = ((float) actualWidth - scaledVirtualWidth) * 0.5f;
        float baseX              = horizontalInset + virtualX * uniformBase;
        float center             = (float) actualWidth * 0.5f;
        return center + (baseX - center) * (1.0f + SCALE_ADJUST);
    }

    inline float ToActualY(float virtualY, int actualWidth, int actualHeight)
    {
        if (actualWidth <= 0 || actualHeight <= 0)
        {
            return virtualY;
        }

        float uniformBase         = std::min(ScaleX(actualWidth), ScaleY(actualHeight));
        float scaledVirtualHeight = HEIGHT * uniformBase;
        float verticalInset       = ((float) actualHeight - scaledVirtualHeight) * 0.5f;
        float baseY               = verticalInset + virtualY * uniformBase;
        float center              = (float) actualHeight * 0.5f;
        return center + (baseY - center) * (1.0f + SCALE_ADJUST);
    }

    inline float ToActualLength(float virtualLength, int actualWidth, int actualHeight)
    {
        return virtualLength * ScaleUniform(actualWidth, actualHeight);
    }

    inline float ToActualOffsetX(float anchorVirtualX, float virtualX, int actualWidth, int actualHeight)
    {
        return ToActualX(anchorVirtualX, actualWidth, actualHeight) + ToActualLength(virtualX - anchorVirtualX, actualWidth, actualHeight);
    }

    inline float ToActualOffsetY(float anchorVirtualY, float virtualY, int actualWidth, int actualHeight)
    {
        return ToActualY(anchorVirtualY, actualWidth, actualHeight) + ToActualLength(virtualY - anchorVirtualY, actualWidth, actualHeight);
    }

    inline Rect ToActualRect(float virtualX, float virtualY, float virtualWidth, float virtualHeight, int actualWidth, int actualHeight)
    {
        return {ToActualX(virtualX, actualWidth, actualHeight), ToActualY(virtualY, actualWidth, actualHeight),
                ToActualLength(virtualWidth, actualWidth, actualHeight), ToActualLength(virtualHeight, actualWidth, actualHeight)};
    }

    inline float ToVirtualX(float actualX, int actualWidth, int actualHeight)
    {
        return (actualX - ToActualX(0.0f, actualWidth, actualHeight)) / ScaleUniform(actualWidth, actualHeight);
    }

    inline float ToVirtualY(float actualY, int actualWidth, int actualHeight)
    {
        return (actualY - ToActualY(0.0f, actualWidth, actualHeight)) / ScaleUniform(actualWidth, actualHeight);
    }

    inline float GetGuiPixelSize(float hudScale, int actualWidth, int actualHeight)
    {
        return std::max(1.0f, roundf(ScaleUniform(actualWidth, actualHeight) * hudScale));
    }

    inline float SnapHudScale(float hudScale, int actualWidth, int actualHeight)
    {
        return GetGuiPixelSize(hudScale, actualWidth, actualHeight) / ScaleUniform(actualWidth, actualHeight);
    }

    inline float SnapVirtualX(float virtualX, int actualWidth, int actualHeight)
    {
        float actualX = ToActualX(virtualX, actualWidth, actualHeight);
        return virtualX + (roundf(actualX) - actualX) / ScaleUniform(actualWidth, actualHeight);
    }

    inline float SnapVirtualY(float virtualY, int actualWidth, int actualHeight)
    {
        float actualY = ToActualY(virtualY, actualWidth, actualHeight);
        return virtualY + (roundf(actualY) - actualY) / ScaleUniform(actualWidth, actualHeight);
    }

    inline int ToActualWidth(float virtualWidth, int actualWidth, int actualHeight)
    {
        int width = (int) lroundf(ToActualLength(virtualWidth, actualWidth, actualHeight));
        return width > 0 ? width : 1;
    }

    inline int ToActualHeight(float virtualHeight, int actualWidth, int actualHeight)
    {
        int height = (int) lroundf(ToActualLength(virtualHeight, actualWidth, actualHeight));
        return height > 0 ? height : 1;
    }
} // namespace Screen
