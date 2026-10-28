#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Screenshot.h"

#include <ctime>

#include "../io/FileSystem.h"
#include "../rendering/RenderDevice.h"

File Screenshot::GetDirectory()
{
    return FileSystem::GetUserDirectory().GetChild("screenshots");
}

File Screenshot::CreateTarget(const std::string &extension)
{
    File directory = GetDirectory();
    directory.CreateDirectories();

    char timestamp[32];
    time_t now = time(nullptr);
    tm local;
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H.%M.%S", &local);

    File target = directory.GetChild(std::string(timestamp) + extension);
    for (int suffix = 1; target.Exists(); suffix++)
    {
        target = directory.GetChild(std::string(timestamp) + "_" + std::to_string(suffix) + extension);
    }

    return target;
}

std::vector<uint8_t> Screenshot::ReadPixels(int width, int height)
{
    std::vector<uint8_t> rgba((size_t) width * (size_t) height * 4);
    RenderDevice::GetInstance()->ReadPixels(0, 0, width, height, rgba.data());
    return rgba;
}

std::vector<uint8_t> Screenshot::ToTopDownRgb(const std::vector<uint8_t> &rgba, int width, int height)
{
    std::vector<uint8_t> rgb((size_t) width * (size_t) height * 3);
    for (int y = 0; y < height; y++)
    {
        const uint8_t *source = rgba.data() + (size_t) (height - 1 - y) * (size_t) width * 4;
        uint8_t *target       = rgb.data() + (size_t) y * (size_t) width * 3;
        for (int x = 0; x < width; x++)
        {
            target[x * 3]     = source[x * 4];
            target[x * 3 + 1] = source[x * 4 + 1];
            target[x * 3 + 2] = source[x * 4 + 2];
        }
    }

    return rgb;
}

std::vector<uint8_t> Screenshot::ToPanoramaFace(const std::vector<uint8_t> &rgba, int width, int height, int size)
{
    std::vector<uint8_t> rgb((size_t) size * (size_t) size * 3);
    for (int y = 0; y < size; y++)
    {
        int sourceY0 = height - 1 - (y + 1) * height / size + 1;
        int sourceY1 = height - 1 - y * height / size;
        for (int x = 0; x < size; x++)
        {
            int sourceX0    = width - (x + 1) * width / size;
            int sourceX1    = width - 1 - x * width / size;
            uint32_t sum[3] = {0, 0, 0};
            uint32_t count  = 0;
            for (int sourceY = sourceY0; sourceY <= sourceY1; sourceY++)
            {
                for (int sourceX = sourceX0; sourceX <= sourceX1; sourceX++)
                {
                    const uint8_t *pixel = rgba.data() + ((size_t) sourceY * (size_t) width + (size_t) sourceX) * 4;
                    sum[0] += pixel[0];
                    sum[1] += pixel[1];
                    sum[2] += pixel[2];
                    count++;
                }
            }

            uint8_t *target = rgb.data() + ((size_t) y * (size_t) size + (size_t) x) * 3;
            target[0]       = (uint8_t) (sum[0] / count);
            target[1]       = (uint8_t) (sum[1] / count);
            target[2]       = (uint8_t) (sum[2] / count);
        }
    }

    return rgb;
}

#endif
