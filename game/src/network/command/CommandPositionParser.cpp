#include "CommandPositionParser.h"

#include <cerrno>
#include <cmath>
#include <cstdlib>

#include "CommandContext.h"

CommandPositionParser::CommandPositionParser()
{}

CommandPositionParser::~CommandPositionParser()
{}

bool CommandPositionParser::ParseVec3(const std::vector<std::string> &values, size_t startIndex, const CommandContext *context, glm::dvec3 *position,
                                      nlohmann::json *errorMessage) const
{
    if (startIndex + 2 >= values.size())
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.pos3d.incomplete");
        }

        return false;
    }

    glm::dvec3 base = context->GetPosition();
    double x        = 0.0;
    double y        = 0.0;
    double z        = 0.0;
    eCommandCoordinateMode xMode;
    eCommandCoordinateMode yMode;
    eCommandCoordinateMode zMode;
    if (!ParseCoordinate(values[startIndex], base.x, &x, &xMode, errorMessage) ||
        !ParseCoordinate(values[startIndex + 1], base.y, &y, &yMode, errorMessage) ||
        !ParseCoordinate(values[startIndex + 2], base.z, &z, &zMode, errorMessage))
    {
        return false;
    }

    bool hasLocal = xMode == eCommandCoordinateMode_Local || yMode == eCommandCoordinateMode_Local || zMode == eCommandCoordinateMode_Local;
    bool allLocal = xMode == eCommandCoordinateMode_Local && yMode == eCommandCoordinateMode_Local && zMode == eCommandCoordinateMode_Local;
    if (hasLocal && !allLocal)
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.pos.mixed");
        }

        return false;
    }

    if (allLocal)
    {
        glm::dvec3 front = context->GetFront();
        glm::dvec3 right = glm::normalize(glm::cross(front, glm::dvec3(0.0, 1.0, 0.0)));
        glm::dvec3 up    = glm::normalize(glm::cross(right, front));
        *position        = context->GetAnchorPosition() + right * -x + up * y + front * z;
        return true;
    }

    *position = glm::dvec3(x, y, z);
    return true;
}

bool CommandPositionParser::ParseBlockPos(const std::vector<std::string> &values, size_t startIndex, const CommandContext *context,
                                          BlockPos *position, nlohmann::json *errorMessage) const
{
    glm::dvec3 exact;
    if (!ParseVec3(values, startIndex, context, &exact, errorMessage))
    {
        return false;
    }

    *position = BlockPos((int) floor(exact.x), (int) floor(exact.y), (int) floor(exact.z));
    return true;
}

bool CommandPositionParser::ParseVec2(const std::vector<std::string> &values, size_t startIndex, const CommandContext *context, double *x, double *z,
                                      nlohmann::json *errorMessage) const
{
    if (startIndex + 1 >= values.size())
    {
        *errorMessage = CommandContext::Translate("argument.pos2d.incomplete");
        return false;
    }

    glm::dvec3 base = context->GetPosition();
    eCommandCoordinateMode xMode;
    eCommandCoordinateMode zMode;
    if (!ParseCoordinate(values[startIndex], base.x, x, &xMode, errorMessage) ||
        !ParseCoordinate(values[startIndex + 1], base.z, z, &zMode, errorMessage))
    {
        return false;
    }

    if (xMode == eCommandCoordinateMode_Local || zMode == eCommandCoordinateMode_Local)
    {
        *errorMessage = CommandContext::Translate("argument.pos.local");
        return false;
    }

    // Whole absolute numbers point at the middle of their block
    if (xMode == eCommandCoordinateMode_Absolute && values[startIndex].find('.') == std::string::npos)
    {
        *x += 0.5;
    }
    if (zMode == eCommandCoordinateMode_Absolute && values[startIndex + 1].find('.') == std::string::npos)
    {
        *z += 0.5;
    }

    return true;
}

bool CommandPositionParser::IsCoordinate(const std::string &text) const
{
    if (text.empty())
    {
        return false;
    }

    if (text[0] == '~' || text[0] == '^')
    {
        if (text.size() == 1)
        {
            return true;
        }

        double value = 0.0;
        return ParseDouble(text.substr(1), &value);
    }

    double value = 0.0;
    return ParseDouble(text, &value);
}

bool CommandPositionParser::ParseCoordinate(const std::string &text, double base, double *value, eCommandCoordinateMode *mode,
                                            nlohmann::json *errorMessage) const
{
    if (text.empty())
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.pos.invalid");
        }

        return false;
    }

    if (text[0] == '~' || text[0] == '^')
    {
        *mode         = text[0] == '~' ? eCommandCoordinateMode_Relative : eCommandCoordinateMode_Local;
        double offset = 0.0;
        if (text.size() > 1 && !ParseDouble(text.substr(1), &offset))
        {
            if (errorMessage)
            {
                *errorMessage = CommandContext::Translate("argument.pos.invalid");
            }

            return false;
        }

        *value = *mode == eCommandCoordinateMode_Relative ? base + offset : offset;
        return true;
    }

    *mode = eCommandCoordinateMode_Absolute;
    if (!ParseDouble(text, value))
    {
        if (errorMessage)
        {
            *errorMessage = CommandContext::Translate("argument.pos.invalid");
        }

        return false;
    }

    return true;
}

bool CommandPositionParser::ParseDouble(const std::string &text, double *value) const
{
    if (text.empty())
    {
        return false;
    }

    errno     = 0;
    char *end = nullptr;
    double d  = strtod(text.c_str(), &end);
    if (end == text.c_str() || *end != '\0' || errno == ERANGE || !std::isfinite(d))
    {
        return false;
    }

    *value = d;
    return true;
}
