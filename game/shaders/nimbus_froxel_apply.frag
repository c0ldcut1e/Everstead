#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_depthTexture;
layout(set = 1, binding = 1) uniform sampler2D u_froxels;
layout(set = 1, binding = 2) uniform sampler2D u_nimbusFlood;
layout(set = 1, binding = 3) uniform sampler2D u_nimbusSpotShadows;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invViewProj;
    vec3 u_froxelGrid;
    float u_maxDistance;
    float u_pointSteps;
    float u_airDensity;
    float u_anisotropy;
    vec2 u_fogRange;
    float u_fogEnabled;
    float u_fogStrength;
    float u_nimbusFog;
    float u_underwaterFog;
    float u_underwaterFogDensity;
    float u_underwaterFogMinimum;
    float u_nimbusLightCount;
    vec4 u_nimbusLightPositions[32];
    vec4 u_nimbusLightColors[32];
    vec4 u_nimbusLightDirections[32];
    vec4 u_nimbusLightParams[32];
    float u_nimbusOcclusion;
    float u_nimbusFloodDecay;
    vec3 u_nimbusFloodOrigin;
    float u_nimbusFloodScatter;
};

#include "nimbus_froxel.glsl"
#include "nimbus_flood.glsl"
#include "nimbus_lights.glsl"
#include "nimbus_scatter.glsl"
#include "nimbus_spot_map.glsl"

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 worldPos = u_invViewProj * vec4(v_uv * 2.0 - 1.0, texture(u_depthTexture, v_uv).r * 2.0 - 1.0, 1.0);
    vec3 position = worldPos.xyz / worldPos.w;
    float depth   = min(length(position), u_maxDistance);
    float last    = u_froxelGrid.z - 1.0;
    float slice   = clamp(log(max(depth, 0.5) / 0.5) / log(u_maxDistance / 0.5) * u_froxelGrid.z - 1.0, 0.0, last);
    int below     = int(slice);
    vec2 cell     = clamp(v_uv * u_froxelGrid.xy, vec2(0.5), u_froxelGrid.xy - 0.5);
    FragColor     = mix(FroxelSample(u_froxels, below, cell), FroxelSample(u_froxels, min(below + 1, int(last)), cell), fract(slice));

    // We march point light scatter per pixel through the flood instead of the froxels, so beams stay sharp at the flood's block resolution
    vec3 rayDir  = normalize(position);
    float dither = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    for (int i = 0; i < int(u_pointSteps); i++)
    {
        float start = float(i) / u_pointSteps;
        float end   = float(i + 1) / u_pointSteps;
        float t     = depth * pow(mix(start, end, dither), 2.0);
        FragColor.rgb += NimbusPointScatter(rayDir * t, rayDir, u_nimbusFloodScatter > 0.5) * (1.0 - WorldFog(t)) * depth * (end * end - start * start);
    }

    // We only march spot beams where the ray crosses the light's range, so the traced steps land inside the beam
    for (int l = 0; l < int(u_nimbusLightCount); l++)
    {
        vec4 lightPosition = u_nimbusLightPositions[l];
        if (abs(u_nimbusLightColors[l].w - 1.0) > 0.5 || u_nimbusLightParams[l].y < 0.5)
        {
            continue;
        }

        float along = dot(lightPosition.xyz, rayDir);
        float miss  = dot(lightPosition.xyz, lightPosition.xyz) - along * along;
        float reach = lightPosition.w * lightPosition.w - miss;
        if (reach <= 0.0)
        {
            continue;
        }

        float enter  = max(along - sqrt(reach), 0.0);
        float exit   = min(along + sqrt(reach), depth);
        float step   = (exit - enter) / u_pointSteps;
        mat3 basis   = NimbusSpotBasis(l);
        float spread = NimbusSpotSpread(l);
        vec2 slot    = vec2(l & 7, l >> 3) * 256.0;
        for (int i = 0; i < int(u_pointSteps) && step > 0.0; i++)
        {
            vec3 p = rayDir * (enter + (float(i) + dither) * step);
            vec3 toLight;
            vec3 color = u_nimbusLightColors[l].rgb * NimbusAttenuation(l, p, toLight);
            vec3 local = (p - lightPosition.xyz) * basis;
            if (dot(color, color) > 0.000001 && local.z > 0.0)
            {
                vec2 texel = slot + clamp((local.xy / (local.z * spread) * 0.5 + 0.5) * 256.0, vec2(0.5), vec2(255.5));
                float hit  = textureLod(u_nimbusSpotShadows, texel / vec2(2048.0, 1024.0), 0.0).r;
                float lit  = clamp(hit - length(local) + 0.5, 0.0, 1.0);
                float fade = 1.0 - WorldFog(length(p));
                FragColor.rgb += color * ((u_airDensity + u_nimbusFog * 1.2 / u_fogRange.y) * Phase(dot(rayDir, toLight)) * step * fade * lit);
            }
        }
    }
}
