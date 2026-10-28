#pragma once

#include <cstdint>
#include <vector>

#include "../../rendering/RenderTypes.h"
#include "../../rendering/Shader.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

class Texture;

class CloudMesh
{
public:
    CloudMesh();
    ~CloudMesh();

    void Upload(const std::vector<float> &vertices);
    void Prepare(Texture *texture, const glm::dmat4 &model, const glm::dmat4 &view, const glm::dmat4 &projection, float red, float green, float blue,
                 float alphaCutoff, float uvOffsetX, float uvOffsetZ, const glm::vec4 &flash) const;
    void Render(int firstVertex, int vertexCount) const;

private:
    Shader m_shader;
    VertexArrayHandle m_vao;
    BufferHandle m_vbo;
    uint32_t m_vertexCount;
};
