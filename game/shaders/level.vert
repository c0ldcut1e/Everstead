#version 450

// We remap GL depth to the Vulkan range here when the device has no depth clip control
layout(constant_id = 0) const bool kRemapDepth = false;

layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 3) in vec4 a_tintMode;
layout(location = 4) in vec4 a_rawLight;
layout(location = 6) in vec4 a_atlasRect;
layout(location = 7) in vec3 a_chunkOffset;

layout(std140, set = 0, binding = 0) uniform VertexParams
{
    mat4 u_model;
    mat4 u_view;
    mat4 u_projection;
    vec3 u_cameraPos;
    vec3 u_chunkScale;
    float u_leafSnow;
};

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_rawLight;
layout(location = 2) out vec3 v_lightPos;
layout(location = 3) out vec3 v_lightScale;
layout(location = 4) out vec3 v_tint;
layout(location = 5) out float v_tintMode;
layout(location = 6) out float v_worldY;
layout(location = 7) out vec4 v_atlasRect;
layout(location = 8) out float v_normalY;
layout(location = 9) out float v_viewDistance;
layout(location = 10) out float v_snowAmount;
layout(location = 11) out float v_snowLeaf;
layout(location = 12) out vec2 v_maskPosition;
layout(location = 13) out vec3 v_localPosition;
layout(location = 14) out vec3 v_normal;
layout(location = 15) out float v_shade;

const float kUvScale = 1024.0;

// The normal rides in position.w as two biased bytes
vec3 DecodeNormal(float packedNormal)
{
    uint bits   = uint(packedNormal);
    vec2 oct    = clamp(vec2(float(bits & 255u), float(bits >> 8u)) / 127.0 - 1.0, -1.0, 1.0);
    vec3 normal = vec3(oct, 1.0 - abs(oct.x) - abs(oct.y));
    if (normal.z < 0.0)
    {
        vec2 signs = vec2(oct.x >= 0.0 ? 1.0 : -1.0, oct.y >= 0.0 ? 1.0 : -1.0);
        normal.xy  = (1.0 - abs(oct.yx)) * signs;
    }

    return normalize(normal);
}

void main()
{
    vec3 localPosition = a_position.xyz * u_chunkScale + a_chunkOffset;
    vec4 viewPos       = u_view * u_model * vec4(localPosition, 1.0);
    gl_Position        = u_projection * viewPos;
    v_uv               = a_uv / kUvScale;
    float tintBits     = floor(a_tintMode.a * 255.0 + 0.5);
    float tintMode     = tintBits >= 128.0 ? 1.0 : 0.0;
    float snowLeaf     = tintBits - tintMode * 128.0 >= 64.0 ? 1.0 : 0.0;
    float shade        = (tintBits - tintMode * 128.0 - snowLeaf * 64.0) / 63.0;
    vec3 normal        = DecodeNormal(a_position.w);
    vec3 lightScale    = vec3(shade);
    // Snowy leaves skip the tint here because level.frag blends it with snow
    if (tintMode <= 0.5 && snowLeaf <= 0.5)
    {
        lightScale *= a_tintMode.rgb;
    }
    v_rawLight   = a_rawLight;
    v_lightPos   = localPosition + normal * 0.5;
    v_maskPosition = localPosition.xz - normal.xz * 0.5;
    v_lightScale = lightScale;
    v_shade      = shade;
    // Every input is the same on all corners of a quad, so the whole face turns white together
    float facing = normal.y > 0.5 ? 1.0 : (normal.y < -0.5 ? 0.45 : 0.8);
    v_snowLeaf   = snowLeaf;
    v_snowAmount = snowLeaf * smoothstep(0.0, 1.0, u_leafSnow) * facing;
    v_tint      = a_tintMode.rgb;
    v_tintMode  = tintMode;
    v_worldY    = viewPos.y + u_cameraPos.y;
    v_atlasRect = a_atlasRect;
    v_normalY   = normal.y;
    v_viewDistance = length(viewPos.xyz);
    v_localPosition = localPosition;
    v_normal        = normal;
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
