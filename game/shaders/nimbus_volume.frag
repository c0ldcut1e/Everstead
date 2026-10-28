#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_nimbusFlood;
layout(set = 1, binding = 1) uniform sampler2D u_nimbusSunCache;
layout(set = 1, binding = 2) uniform usampler2D u_nimbusGrid;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invViewProj;
    vec3 u_cameraPos;
    float u_time;
    float u_maxDistance;
    float u_airDensity;
    float u_anisotropy;
    float u_fogCount;
    vec4 u_fogPositions[8];
    vec4 u_fogExtents[8];
    vec4 u_fogColors[8];
    vec4 u_fogParams[8];
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
    vec3 u_nimbusGridOrigin;
    float u_nimbusOcclusion;
    vec3 u_nimbusGridWrap;
    float u_nimbusFloodDecay;
    vec3 u_nimbusFloodOrigin;
    vec3 u_froxelGrid;
};

layout(location = 0) out vec4 FragColor;

#include "nimbus_trace.glsl"
#include "nimbus_flood.glsl"
#include "nimbus_froxel.glsl"
#include "nimbus_lights.glsl"
#include "nimbus_sun_cache.glsl"
#include "nimbus_scatter.glsl"

float Hash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float ValueNoise(vec3 p)
{
    vec3 cell = floor(p);
    vec3 f    = fract(p);
    f         = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(Hash(cell), Hash(cell + vec3(1, 0, 0)), f.x), mix(Hash(cell + vec3(0, 1, 0)), Hash(cell + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(Hash(cell + vec3(0, 0, 1)), Hash(cell + vec3(1, 0, 1)), f.x), mix(Hash(cell + vec3(0, 1, 1)), Hash(cell + vec3(1, 1, 1)), f.x),
                   f.y),
               f.z);
}

float FogDensity(int index, vec3 p)
{
    vec3 local  = (p - u_fogPositions[index].xyz) / u_fogExtents[index].xyz;
    vec3 q      = abs(local);
    float shape = u_fogPositions[index].w > 0.5 ? length(local) : max(max(q.x, q.y), q.z);
    if (shape >= 1.0)
    {
        return 0.0;
    }

    vec4 params   = u_fogParams[index];
    float density = u_fogExtents[index].w * smoothstep(0.0, 1.0, (1.0 - shape) / max(u_fogColors[index].w, 0.001));
    if (params.x > 0.0)
    {
        vec3 world = (p + u_cameraPos) * 0.15 + vec3(1.0, 0.2, 0.5) * u_time * params.y;
        density *= mix(1.0, ValueNoise(world) * 0.65 + ValueNoise(world * 2.03) * 0.35, params.x);
    }

    return density;
}

// Each texel is one froxel, it stores the light scattered per block and the fog extinction at the froxel's center
void main()
{
    ivec3 froxel  = FroxelAt(ivec2(gl_FragCoord.xy));
    vec2 uv       = (vec2(froxel.xy) + 0.5) / u_froxelGrid.xy;
    vec4 far      = u_invViewProj * vec4(uv * 2.0 - 1.0, 1.0, 1.0);
    vec3 rayDir   = normalize(far.xyz / far.w);
    float depth   = (FroxelDepth(float(froxel.z)) + FroxelDepth(float(froxel.z + 1))) * 0.5;
    vec3 p        = rayDir * depth;
    float air     = NimbusTint(p);

    float fogDensity = 0.0;
    vec3 fogAlbedo   = vec3(0.0);
    vec3 emission    = vec3(0.0);
    for (int f = 0; f < int(u_fogCount); f++)
    {
        float density = FogDensity(f, p);
        fogDensity += density;
        fogAlbedo += u_fogColors[f].rgb * density;
        emission += u_fogColors[f].rgb * density * u_fogParams[f].z;
    }

    fogAlbedo /= max(fogDensity, 0.000001);
    vec3 light          = emission;
    vec3 floodedPoint   = vec3(0.0);
    float expectedPoint = 0.0;
    float haze = u_nimbusFog * 1.2 / u_fogRange.y;
    for (int l = 0; l < int(u_nimbusLightCount) && air > 0.0; l++)
    {
        vec4 lightPosition = u_nimbusLightPositions[l];
        float type         = u_nimbusLightColors[l].w;
        bool occluded      = u_nimbusLightParams[l].w > 0.5;
        if (type < 1.5 && dot(p - lightPosition.xyz, p - lightPosition.xyz) >= lightPosition.w * lightPosition.w)
        {
            continue;
        }

        vec3 toLight;
        vec3 color = u_nimbusLightColors[l].rgb * NimbusAttenuation(l, p, toLight);
        if (type < 0.5 && occluded)
        {
            expectedPoint += dot(color, color);
        }

        bool volumetric = u_nimbusLightParams[l].y > 0.5 && type > 1.5;
        if ((!volumetric && fogDensity <= 0.0 && haze <= 0.0) || dot(color, color) < 0.000001)
        {
            continue;
        }

        vec3 scattered = color * (fogAlbedo * fogDensity + vec3((volumetric ? u_airDensity : 0.0) + haze)) * Phase(dot(rayDir, toLight));
        if (!occluded)
        {
            light += scattered;
        }
        else if (type > 1.5)
        {
            light += scattered * NimbusSunVisibility(l, p);
        }
        else if (type > 0.5)
        {
            // Spot beams are traced because the flood spreads around corners and would show the beam on the far side of a wall
            light += scattered * NimbusTrace(p, u_nimbusLightPositions[l].xyz);
        }
        else
        {
            floodedPoint += scattered;
        }
    }

    if (expectedPoint > 0.0)
    {
        vec4 flood = NimbusFloodSample(p);
        light += floodedPoint * FloodVisibility(flood.r + flood.g + flood.b, expectedPoint, NimbusFloodFade(p));
    }

    float unfogged = 1.0 - WorldFog(depth);
    FragColor      = vec4(light * air * unfogged, fogDensity * unfogged);
}
