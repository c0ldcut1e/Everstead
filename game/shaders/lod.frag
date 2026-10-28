#version 450

layout(location = 0) in vec3 v_color;
layout(location = 1) in vec2 v_maskPosition;
layout(location = 2) in float v_viewDistance;
layout(location = 3) in vec2 v_uv;
layout(location = 4) flat in vec4 v_atlasRect;
layout(location = 5) flat in vec3 v_tint;
layout(location = 6) flat in vec4 v_ao;
layout(location = 7) in vec2 v_corner;
layout(location = 8) flat in vec2 v_fadeRange;
layout(location = 9) flat in vec3 v_debugColor;

layout(set = 1, binding = 0) uniform sampler2D u_texture;
layout(set = 1, binding = 2) uniform sampler2D u_lodMask;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_skyDarken;
    float u_darkAdaptation;
    vec2 u_fogRange;
    vec3 u_fogColor;
    float u_lodMaskEnabled;
    vec2 u_lodMaskOrigin;
    float u_lodMaskSize;
    float u_lodTextures;
    float u_lodMipLevels;
    vec3 u_hazeColor;
    vec2 u_hazeRange;
};

layout(location = 0) out vec4 FragColor;

void main()
{
    float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    if (noise < v_fadeRange.x || noise >= v_fadeRange.y)
    {
        discard;
    }

    if (u_lodMaskEnabled > 0.5)
    {
        ivec2 texel = ivec2(floor((v_maskPosition - u_lodMaskOrigin) / 16.0));
        int size    = int(u_lodMaskSize);
        if (texel.x >= 0 && texel.y >= 0 && texel.x < size && texel.y < size && noise < texelFetch(u_lodMask, texel, 0).r)
        {
            discard;
        }
    }

    vec3 color = v_color;
    if (u_lodTextures > 0.5)
    {
        vec2 atlasSize = vec2(textureSize(u_texture, 0));
        vec2 cellSize  = (v_atlasRect.zw - v_atlasRect.xy) * atlasSize;
        vec2 texelX    = dFdx(v_uv) * cellSize;
        vec2 texelY    = dFdy(v_uv) * cellSize;
        // We pick the mip from the unwrapped uv because fract breaks the hardware derivatives at block edges
        float lod     = clamp(0.5 * log2(max(dot(texelX, texelX), dot(texelY, texelY))), 0.0, u_lodMipLevels);
        vec2 inset    = vec2(0.01) / atlasSize;
        vec2 uv       = mix(v_atlasRect.xy + inset, v_atlasRect.zw - inset, fract(v_uv));
        vec4 tex      = textureLod(u_texture, uv, lod);
        color         = mix(v_color, tex.rgb * v_tint, tex.a);
    }

    // We blend the corner shading per pixel because per vertex shading splits each quad into two visible triangles
    color *= mix(mix(v_ao.x, v_ao.y, v_corner.y), mix(v_ao.w, v_ao.z, v_corner.y), v_corner.x);

    float skyDarken = clamp(u_skyDarken, 0.0, 1.0);
    float skyBase   = (skyDarken * 0.95 + 0.05) * (1.0 - u_darkAdaptation * 0.4);
    float skyWarm   = skyBase * (skyDarken * 0.65 + 0.35);
    float ambient   = 0.03 * (1.0 - u_darkAdaptation);
    vec3 light      = vec3(skyWarm, skyWarm, skyBase) * 0.96 + ambient;
    light           = clamp(light * 0.96 + ambient, vec3(0.0), vec3(1.0));
    float fog       = clamp((v_viewDistance - u_fogRange.x) / (u_fogRange.y - u_fogRange.x), 0.0, 1.0);
    float haze      = smoothstep(u_hazeRange.x, u_hazeRange.y, v_viewDistance) * 0.35;
    vec3 lit        = mix(color * v_debugColor * light, u_hazeColor, haze);
    FragColor       = vec4(mix(lit, u_fogColor, fog), 1.0);
}
