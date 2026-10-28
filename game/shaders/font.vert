#version 450

// We remap GL depth to the Vulkan range here when the device has no depth clip control
layout(constant_id = 0) const bool kRemapDepth = false;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(std140, set = 0, binding = 0) uniform VertexParams
{
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;

void main()
{
    v_uv        = a_uv;
    v_color     = a_color;
    gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
