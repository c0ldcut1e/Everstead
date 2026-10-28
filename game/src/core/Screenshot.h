#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../io/File.h"

class Screenshot
{
public:
    static File GetDirectory();
    static File CreateTarget(const std::string &extension);
    static std::vector<uint8_t> ReadPixels(int width, int height);
    static std::vector<uint8_t> ToTopDownRgb(const std::vector<uint8_t> &rgba, int width, int height);
    static std::vector<uint8_t> ToPanoramaFace(const std::vector<uint8_t> &rgba, int width, int height, int size);
};
