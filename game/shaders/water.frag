#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_rawLight;
layout(location = 2) in vec3 v_lightPos;
layout(location = 3) in vec3 v_lightScale;
layout(location = 7) in vec4 v_atlasRect;
layout(location = 8) in float v_normalY;
layout(location = 9) in float v_viewDistance;
layout(location = 12) in vec2 v_maskPosition;
layout(location = 13) in vec3 v_localPosition;
layout(location = 14) in vec3 v_normal;
layout(location = 15) in float v_shade;

layout(set = 1, binding = 0) uniform sampler2D u_texture;
layout(set = 1, binding = 2) uniform sampler2D u_lodMask;
layout(set = 1, binding = 3) uniform sampler2D u_sceneDepth;
layout(set = 1, binding = 4) uniform usampler2D u_nimbusTiles;
layout(set = 1, binding = 5) uniform sampler2D u_nimbusFlood;
layout(set = 1, binding = 6) uniform usampler2D u_nimbusGrid;
layout(set = 1, binding = 7) uniform sampler2D u_nimbusSunCache;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    vec2 u_fogRange;
    float u_fogEnabled;
    float u_fogStrength;
    float u_nimbusFog;
    vec3 u_fogColor;
    float u_underwaterFog;
    vec3 u_underwaterFogColor;
    float u_underwaterFogDensity;
    float u_underwaterFogMinimum;
    float u_skyDarken;
    float u_blockLightScale;
    float u_darkAdaptation;
    float u_itemLightCount;
    vec4 u_itemLightPositions[32];
    vec4 u_itemLightColors[32];
    float u_lodMaskEnabled;
    vec2 u_lodMaskOrigin;
    float u_lodMaskSize;
    float u_nimbusSurface;
    float u_nimbusLightCount;
    vec4 u_nimbusLightPositions[32];
    vec4 u_nimbusLightColors[32];
    vec4 u_nimbusLightDirections[32];
    vec4 u_nimbusLightParams[32];
    vec3 u_nimbusFloodOrigin;
    float u_nimbusFloodLights;
    vec3 u_nimbusGridOrigin;
    float u_nimbusOcclusion;
    vec3 u_nimbusGridWrap;
    float u_nimbusTilePixels;
    float u_nimbusMode;
    float u_nimbusBounce;
    float u_nimbusFloodDecay;
    float u_nimbusLighting;
};

layout(location = 0) out vec4 FragColor;

float BrightnessRamp(float level)
{
    float clamped = clamp(level, 0.0, 1.0);
    float v       = 1.0 - clamped;
    return (1.0 - v) / (v * 3.0 + 1.0);
}

vec3 SampleLceLight(vec4 rawLight)
{
    float skyDarken = clamp(u_skyDarken, 0.0, 1.0);
    float darken    = skyDarken * 0.95 + 0.05;
    float skyBase   = BrightnessRamp(rawLight.w) * darken * (1.0 - u_darkAdaptation * 0.4);
    float skyWarm   = skyBase * (skyDarken * 0.65 + 0.35);

    float blockScale = u_blockLightScale;
    float rb         = BrightnessRamp(rawLight.x) * blockScale;
    float gb         = BrightnessRamp(rawLight.y) * blockScale;
    float bb         = BrightnessRamp(rawLight.z) * blockScale;

    vec3 light;
    light.r = skyWarm + rb;
    light.g = skyWarm + gb * ((gb * 0.6 + 0.4) * 0.6 + 0.4);
    light.b = skyBase + bb * ((bb * bb) * 0.6 + 0.4);

    float ambientFloor = 0.03 * (1.0 - u_darkAdaptation);
    light              = light * 0.96 + ambientFloor;
    light              = light * 0.96 + ambientFloor;

    return clamp(light, vec3(0.0), vec3(1.0));
}

vec4 ApplyItemLights(vec4 rawLight, vec3 samplePos)
{
    for (int i = 0; i < int(u_itemLightCount); i++)
    {
        float falloff = distance(samplePos, u_itemLightPositions[i].xyz) / 15.0;
        rawLight.rgb  = max(rawLight.rgb, u_itemLightColors[i].rgb - falloff);
    }

    return rawLight;
}

#include "nimbus_trace.glsl"
#include "world_fog.glsl"

#include "nimbus_flood.glsl"
#include "nimbus_lights.glsl"
#include "nimbus_sun_cache.glsl"
#include "nimbus_surface.glsl"

vec2 AtlasUv(vec2 uv)
{
    vec2 wrapped = fract(uv);
    if (wrapped.x == 0.0 && uv.x > 0.0)
    {
        wrapped.x = 1.0;
    }
    if (wrapped.y == 0.0 && uv.y > 0.0)
    {
        wrapped.y = 1.0;
    }
    vec2 atlasSize = vec2(textureSize(u_texture, 0));
    vec2 inset     = vec2(0.01) / max(atlasSize, vec2(1.0));
    vec2 minUv     = v_atlasRect.xy + inset;
    vec2 maxUv     = v_atlasRect.zw - inset;
    return mix(minUv, maxUv, wrapped);
}

void main()
{
    if (gl_FragCoord.z > texelFetch(u_sceneDepth, ivec2(gl_FragCoord.xy), 0).r)
    {
        discard;
    }

    if (u_lodMaskEnabled > 0.5)
    {
        ivec2 texel = ivec2(floor((v_maskPosition - u_lodMaskOrigin) / 16.0));
        int size    = int(u_lodMaskSize);
        float noise = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
        if (texel.x >= 0 && texel.y >= 0 && texel.x < size && texel.y < size && noise >= texelFetch(u_lodMask, texel, 0).r)
        {
            discard;
        }
    }

    vec2 atlasSpan = v_atlasRect.zw - v_atlasRect.xy;
    vec2 gradX     = dFdx(v_uv) * atlasSpan;
    vec2 gradY     = dFdy(v_uv) * atlasSpan;
    vec4 tex       = textureGrad(u_texture, AtlasUv(v_uv), gradX, gradY);
    if (tex.a < 0.1)
    {
        discard;
    }
    vec3 unshadedLight = SampleLceLight(
            u_nimbusLighting > 0.5 ? mix(vec4(0.0, 0.0, 0.0, v_rawLight.w), ApplyItemLights(v_rawLight, v_lightPos), NimbusFloodFade(v_localPosition))
                                   : ApplyItemLights(v_rawLight, v_lightPos));
    vec3 light = unshadedLight * v_lightScale;
    vec3 color = tex.rgb * light;
    if (u_nimbusSurface > 0.5)
    {
        color += tex.rgb * v_shade * NimbusSurfaceLight(v_localPosition, gl_FrontFacing ? normalize(v_normal) : -normalize(v_normal), unshadedLight);
    }

    if (u_underwaterFog > 0.5)
    {
        float underwaterFog = max(1.0 - exp(-u_underwaterFogDensity * v_viewDistance), u_underwaterFogMinimum);
        color               = mix(color, u_underwaterFogColor, clamp(underwaterFog, 0.0, 1.0));
    }
    else if (u_fogEnabled > 0.5)
    {
        float fogValue = DistanceFog(v_viewDistance);
        color          = mix(color, u_fogColor, fogValue);
    }

    FragColor = vec4(color, tex.a);
}
