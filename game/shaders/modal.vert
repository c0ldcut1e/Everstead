#version 450

// We remap GL depth to the Vulkan range here when the device has no depth clip control
layout(constant_id = 0) const bool kRemapDepth = false;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;

layout(std140, set = 0, binding = 0) uniform VertexParams
{
    vec2 u_resolution;
};

layout(location = 0) out vec2 v_position;
layout(location = 1) out vec2 v_uv;

void main()
{
    v_position = a_position;
    v_uv       = a_uv;

    vec2 ndc;
    ndc.x       = (a_position.x / u_resolution.x) * 2.0 - 1.0;
    ndc.y       = 1.0 - (a_position.y / u_resolution.y) * 2.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
