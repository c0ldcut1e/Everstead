#version 450

// We remap GL depth to the Vulkan range here when the device has no depth clip control
layout(constant_id = 0) const bool kRemapDepth = false;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec4 a_atlasRect;
layout(location = 3) in vec4 a_tint;
layout(location = 4) in vec4 a_ao;
layout(location = 5) in vec4 a_tileOrigin;
layout(location = 6) in vec4 a_tileStyle;

layout(std140, set = 0, binding = 0) uniform VertexParams
{
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
};

layout(location = 0) out vec3 v_color;
layout(location = 1) out vec2 v_maskPosition;
layout(location = 2) out float v_viewDistance;
layout(location = 3) out vec2 v_uv;
layout(location = 4) flat out vec4 v_atlasRect;
layout(location = 5) flat out vec3 v_tint;
layout(location = 6) flat out vec4 v_ao;
layout(location = 7) out vec2 v_corner;
layout(location = 8) flat out vec2 v_fadeRange;
layout(location = 9) flat out vec3 v_debugColor;

const vec3 kFaceNormals[7] = vec3[](vec3(0.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(-1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0), vec3(0.0, 0.0, -1.0), vec3(0.0, 1.0, 0.0), vec3(0.0, -1.0, 0.0));

void main()
{
    vec3 localPosition = a_position.xyz + a_tileOrigin.xyz;
    // Water tops sit at the same height as still water in chunks so the two surfaces meet without a lip
    if (int(a_position.w) == 5)
    {
        localPosition.y -= 1.0 / 9.0 + 0.001;
    }

    vec4 viewPos       = u_view * u_model * vec4(localPosition, 1.0);
    gl_Position        = u_projection * viewPos;
    v_color            = a_color.rgb;
    v_maskPosition     = localPosition.xz - kFaceNormals[int(a_position.w)].xz * 0.5;
    v_viewDistance     = length(viewPos.xyz);
    v_atlasRect        = a_atlasRect;
    v_tint             = a_tint.rgb;
    v_ao               = a_ao;
    v_fadeRange        = vec2(a_tileOrigin.w, a_tileStyle.x);
    v_debugColor       = a_tileStyle.yzw;
    int corner         = gl_VertexIndex % 4;
    v_corner           = vec2(corner >= 2 ? 1.0 : 0.0, corner == 1 || corner == 2 ? 1.0 : 0.0);
    int face           = int(a_position.w);
    if (face == 0 || face >= 5)
    {
        v_uv = a_position.xz;
    }
    else if (face <= 2)
    {
        v_uv = vec2(a_position.z, -a_position.y);
    }
    else
    {
        v_uv = vec2(a_position.x, -a_position.y);
    }
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
