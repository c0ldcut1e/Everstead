#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "RenderTypes.h"

class Shader
{
public:
    Shader();
    Shader(const char *vertexPath, const char *fragmentPath);
    ~Shader();

    void Load(const char *vertexPath, const char *fragmentPath);
    void Bind() const;

    void SetInt(const char *name, int value) const;
    void SetFloat(const char *name, float value) const;
    void SetVec2(const char *name, float x, float y) const;
    void SetVec3(const char *name, float x, float y, float z) const;
    void SetVec4(const char *name, float x, float y, float z, float w) const;
    void SetVec4Array(const char *name, int count, const float *data) const;
    void SetMat4(const char *name, const double *data) const;

private:
    struct UniformHash
    {
        using is_transparent = void;

        size_t operator()(std::string_view name) const
        {
            return std::hash<std::string_view>()(name);
        }
    };

    struct UniformSlot
    {
        int block;
        int offset;
    };

    void LoadReflection(const char *path, std::vector<ShaderUniformBlock> *blocks, std::vector<ShaderSampler> *samplers);
    void SetValues(const char *name, const float *values, int count) const;

    ProgramHandle m_program;
    std::unordered_map<std::string, std::vector<UniformSlot>, UniformHash, std::equal_to<>> m_uniforms;
    mutable std::vector<std::vector<float>> m_blockData;
};
