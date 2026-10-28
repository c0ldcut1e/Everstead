#version 450

// We remap GL depth to the Vulkan range here when the device has no depth clip control
layout(constant_id = 0) const bool kRemapDepth = false;

layout(location = 0) in vec3 a_position;

layout(std140, set = 0, binding = 0) uniform VertexParams
{
    mat4 u_viewRot;
    mat4 u_projection;
    mat4 u_model;
};

void main()
{
    gl_Position = u_projection * u_viewRot * u_model * vec4(a_position, 1.0);
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
