#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "../../world/block/BlockPos.h"

#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

class CommandContext;

enum eCommandCoordinateMode
{
    eCommandCoordinateMode_Absolute,
    eCommandCoordinateMode_Relative,
    eCommandCoordinateMode_Local,
};

class CommandPositionParser
{
public:
    CommandPositionParser();
    ~CommandPositionParser();

    bool ParseVec3(const std::vector<std::string> &values, size_t startIndex, const CommandContext *context, glm::dvec3 *position,
                   nlohmann::json *errorMessage) const;
    bool ParseBlockPos(const std::vector<std::string> &values, size_t startIndex, const CommandContext *context, BlockPos *position,
                       nlohmann::json *errorMessage) const;
    bool ParseVec2(const std::vector<std::string> &values, size_t startIndex, const CommandContext *context, double *x, double *z,
                   nlohmann::json *errorMessage) const;
    bool IsCoordinate(const std::string &text) const;

private:
    bool ParseCoordinate(const std::string &text, double base, double *value, eCommandCoordinateMode *mode, nlohmann::json *errorMessage) const;
    bool ParseDouble(const std::string &text, double *value) const;
};
