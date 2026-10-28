#ifndef EVERSTEAD_DEDICATED_SERVER

#include "Shader.h"

#include <cstring>
#include <string>

#include "../io/File.h"
#include "RenderDevice.h"

#include <nlohmann/json.hpp>

Shader::Shader()
{}

Shader::Shader(const char *vertexPath, const char *fragmentPath)
{
    Load(vertexPath, fragmentPath);
}

Shader::~Shader()
{
    // Static owners outlive the render device, and their programs were already freed with it
    RenderDevice *device = RenderDevice::GetInstance();
    if (device && m_program.IsValid())
    {
        device->DeleteProgram(m_program);
    }
}

void Shader::Load(const char *vertexPath, const char *fragmentPath)
{
    std::vector<ShaderUniformBlock> blocks;
    std::vector<ShaderSampler> samplers;
    LoadReflection(vertexPath, &blocks, &samplers);
    LoadReflection(fragmentPath, &blocks, &samplers);

    m_blockData.resize(blocks.size());
    for (size_t i = 0; i < blocks.size(); i++)
    {
        m_blockData[i].assign(blocks[i].size / sizeof(float), 0.0f);
    }

    m_program = RenderDevice::GetInstance()->CreateProgram(vertexPath, fragmentPath, blocks, samplers);
}

void Shader::LoadReflection(const char *path, std::vector<ShaderUniformBlock> *blocks, std::vector<ShaderSampler> *samplers)
{
    nlohmann::json reflection = nlohmann::json::parse(File(std::string(path) + ".json").ReadAllText());

    if (reflection.contains("ubos"))
    {
        for (const nlohmann::json &ubo : reflection["ubos"])
        {
            std::string name = ubo["name"].get<std::string>();
            int block        = 0;
            while (block < (int) blocks->size() && (*blocks)[block].name != name)
            {
                block++;
            }

            if (block < (int) blocks->size())
            {
                continue;
            }

            // We round block sizes up to a vec4 because the device uploads whole elements
            uint32_t size = ubo["block_size"].get<uint32_t>();
            blocks->push_back(ShaderUniformBlock{name, (size + 15) / 16 * 16});
            for (const nlohmann::json &member : reflection["types"][ubo["type"].get<std::string>()]["members"])
            {
                m_uniforms[member["name"].get<std::string>()].push_back(UniformSlot{block, member["offset"].get<int>()});
            }
        }
    }

    if (reflection.contains("textures"))
    {
        for (const nlohmann::json &texture : reflection["textures"])
        {
            samplers->push_back(ShaderSampler{texture["name"].get<std::string>(), texture["binding"].get<uint32_t>()});
        }
    }
}

void Shader::Bind() const
{
    RenderDevice::GetInstance()->UseProgram(m_program);
}

void Shader::SetValues(const char *name, const float *values, int count) const
{
    auto it = m_uniforms.find(std::string_view(name));
    if (it == m_uniforms.end())
    {
        return;
    }

    for (const UniformSlot &slot : it->second)
    {
        float *destination = m_blockData[slot.block].data() + slot.offset / 4;
        if (memcmp(destination, values, count * sizeof(float)) == 0)
        {
            continue;
        }

        memcpy(destination, values, count * sizeof(float));
        int firstVec4 = slot.offset / 16;
        int lastVec4  = (slot.offset + count * 4 + 15) / 16;
        RenderDevice::GetInstance()->UploadUniformBlock(m_program, slot.block, firstVec4, lastVec4 - firstVec4,
                                                        m_blockData[slot.block].data() + firstVec4 * 4);
    }
}

void Shader::SetInt(const char *name, int value) const
{
    float converted = (float) value;
    SetValues(name, &converted, 1);
}

void Shader::SetFloat(const char *name, float value) const
{
    SetValues(name, &value, 1);
}

void Shader::SetVec2(const char *name, float x, float y) const
{
    float values[2] = {x, y};
    SetValues(name, values, 2);
}

void Shader::SetVec3(const char *name, float x, float y, float z) const
{
    float values[3] = {x, y, z};
    SetValues(name, values, 3);
}

void Shader::SetVec4(const char *name, float x, float y, float z, float w) const
{
    float values[4] = {x, y, z, w};
    SetValues(name, values, 4);
}

void Shader::SetVec4Array(const char *name, int count, const float *data) const
{
    SetValues(name, data, count * 4);
}

void Shader::SetMat4(const char *name, const double *data) const
{
    float matrix[16];
    for (int i = 0; i < 16; i++)
    {
        matrix[i] = (float) data[i];
    }

    SetValues(name, matrix, 16);
}

#endif
