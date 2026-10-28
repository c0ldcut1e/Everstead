#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_colorTexture;
layout(set = 1, binding = 1) uniform sampler2D u_depthTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    vec2 u_fogRange;
    float u_fogEnabled;
    mat4 u_invViewProj;
    float u_fogStrength;
    vec3 u_fogColor;
    float u_skyFog;
    float u_underwaterFog;
    vec3 u_underwaterFogColor;
    float u_underwaterFogDensity;
    float u_underwaterFogMinimum;
    float u_nimbusFog;
};

layout(location = 0) out vec4 FragColor;

#include "world_fog.glsl"

float ReconstructViewDistance(float depthSample)
{
    float ndcZ    = depthSample * 2.0 - 1.0;
    vec2 ndcXY    = v_uv * 2.0 - 1.0;
    vec4 clipPos  = vec4(ndcXY, ndcZ, 1.0);
    vec4 worldPos = u_invViewProj * clipPos;
    worldPos /= worldPos.w;
    return length(worldPos.xyz);
}

void main()
{
    vec4 sceneColor   = texture(u_colorTexture, v_uv);
    float depthSample = texture(u_depthTexture, v_uv).r;

    if (u_underwaterFog > 0.5)
    {
        float underwaterDist = ReconstructViewDistance(depthSample);
        float underwaterFog  = 1.0 - exp(-u_underwaterFogDensity * underwaterDist);
        underwaterFog        = max(underwaterFog, u_underwaterFogMinimum);
        FragColor = vec4(mix(sceneColor.rgb, u_underwaterFogColor, clamp(underwaterFog, 0.0, 1.0)),
                         1.0);
        return;
    }

    if (u_fogEnabled < 0.5 || u_fogStrength <= 0.0001 || (depthSample >= 1.0 && u_skyFog <= 0.0))
    {
        FragColor = sceneColor;
        return;
    }

    float viewDist = ReconstructViewDistance(depthSample);
    float fogValue = DistanceFog(viewDist);

    if (depthSample >= 1.0)
    {
        fogValue = clamp(u_skyFog, 0.0, 1.0);
    }

    vec3 rgb  = mix(sceneColor.rgb, u_fogColor, fogValue);
    FragColor = vec4(rgb, 1.0);
}
