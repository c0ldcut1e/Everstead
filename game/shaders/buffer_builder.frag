#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) in float v_viewZ;

layout(set = 1, binding = 0) uniform sampler2D u_texture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_useTexture;
    float u_alphaCutoff;
    vec4 u_overlayColor;
    float u_fogEnabled;
    vec3 u_fogColor;
    vec2 u_fogRange;
    float u_glint;
    float u_time;
};

layout(location = 0) out vec4 FragColor;

vec2 Hash2(vec2 p)
{
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

// We work on whole texels so the glint stays pixel art like the item it sits on
vec3 Glint(float level)
{
    vec2 texel  = floor(v_uv * vec2(textureSize(u_texture, 0)));
    float phase = (texel.x + texel.y) / 40.0 - u_time * 0.18;
    float wave  = fract(phase);
    vec2 hash   = Hash2(texel + floor(phase) * 17.0);
    float band  = smoothstep(0.0, 0.1, wave) * smoothstep(0.35, 0.1, wave);
    float lit   = step(hash.x, (0.08 + 0.14 * level) * band);
    float flick = 0.6 + 0.4 * sin(u_time * 9.0 + hash.y * 6.28);
    vec3 shard  = mix(vec3(0.45, 0.9, 1.0), vec3(0.7, 0.45, 1.0), hash.y);
    return shard * lit * flick * (0.7 + 0.5 * level) + vec3(0.55, 0.35, 1.0) * band * (0.05 + 0.05 * level);
}

void main()
{
    vec4 color = v_color;

    if (u_useTexture > 0.5)
    {
        vec4 tex = texture(u_texture, v_uv);
        if (tex.a <= u_alphaCutoff)
        {
            discard;
        }
        color *= vec4(mix(tex.rgb, u_overlayColor.rgb, u_overlayColor.a), tex.a);
    }

    vec3 glint = u_glint > 0.0 && u_useTexture > 0.5 ? Glint(u_glint) * color.a : vec3(0.0);

    if (u_fogEnabled > 0.5)
    {
        float fogStart      = u_fogRange.x;
        float fogEnd        = max(u_fogRange.y, fogStart + 0.0001);
        float v_fogDistance = abs(v_viewZ);
        float fogFactor     = clamp((v_fogDistance - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
        color.rgb           = mix(color.rgb, u_fogColor, fogFactor);
        glint *= 1.0 - fogFactor;
    }

    color.rgb += glint;

    FragColor = color;
}
