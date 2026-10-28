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
    vec3 u_cloudColor;
    vec2 u_uvOffset;
    vec4 u_flash;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;
layout(location = 2) out float v_viewZ;

const float kFlashRadius = 96.0;

void main()
{
    vec4 position = u_model * vec4(a_position, 1.0);
    vec4 viewPosition = u_view * position;
    gl_Position = u_projection * viewPosition;
    v_uv = a_uv + u_uvOffset;
    float flash = max(1.0 - length(position.xyz - u_flash.xyz) / kFlashRadius, 0.0);
    v_color = vec4(mix(a_color.rgb * u_cloudColor, vec3(0.85, 0.88, 1.0), u_flash.w * flash * flash), a_color.a);
    v_viewZ = viewPosition.z;
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
