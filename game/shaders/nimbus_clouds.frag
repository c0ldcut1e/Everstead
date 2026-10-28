#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_depthTexture;
layout(set = 1, binding = 1) uniform sampler2D u_cloudTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invViewProj;
    vec4 u_flash;
    vec3 u_cloudColor;
    float u_cloudBottom;
    vec3 u_sunDirection;
    float u_rain;
    vec3 u_fogColor;
    float u_maxDistance;
    vec2 u_cloudCell;
    vec2 u_fogRange;
    float u_fogEnabled;
    float u_fogStrength;
    float u_nimbusFog;
    float u_time;
    float u_steps;
    float u_underwaterFog;
    vec3 u_underwaterFogColor;
    float u_underwaterFogDensity;
    float u_underwaterFogMinimum;
};

layout(location = 0) out vec4 FragColor;

#include "world_fog.glsl"

const float kThickness  = 8.0;
const float kCellSize   = 12.0;
const float kExtinction = 0.35;

float Hash(vec3 p)
{
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

float Noise(vec3 p)
{
    vec3 i = floor(p);
    vec3 f = fract(p);
    f      = f * f * (3.0 - 2.0 * f);
    return mix(mix(mix(Hash(i), Hash(i + vec3(1, 0, 0)), f.x), mix(Hash(i + vec3(0, 1, 0)), Hash(i + vec3(1, 1, 0)), f.x), f.y),
               mix(mix(Hash(i + vec3(0, 0, 1)), Hash(i + vec3(1, 0, 1)), f.x), mix(Hash(i + vec3(0, 1, 1)), Hash(i + vec3(1, 1, 1)), f.x), f.y), f.z);
}

float Texel(ivec2 cell)
{
    return step(0.1, texelFetch(u_cloudTexture, cell & 255, 0).a);
}

// We filter the cloud map by hand because it is sampled per cell and has to stay nearest for the mesh clouds
float Coverage(vec2 cell)
{
    vec2 base = cell - 0.5;
    ivec2 i   = ivec2(floor(base));
    vec2 f    = smoothstep(0.0, 1.0, fract(base));
    return mix(mix(Texel(i), Texel(i + ivec2(1, 0)), f.x), mix(Texel(i + ivec2(0, 1)), Texel(i + ivec2(1, 1)), f.x), f.y);
}

float Density(vec3 p)
{
    float h = clamp((p.y - u_cloudBottom) / kThickness, 0.0, 1.0);
    float c = Coverage(u_cloudCell + p.xz / kCellSize) + u_rain * 0.25;
    if (c <= 0.05)
    {
        return 0.0;
    }

    float n = Noise(p * 0.11 + vec3(u_time * 0.05, 0.0, u_time * 0.02)) * 0.65 + Noise(p * 0.31) * 0.35;
    float d = c - 0.35 - (h * 2.0 - 1.0) * (h * 2.0 - 1.0) * 0.55 + (n - 0.5) * 0.35;
    return clamp(d * 3.0, 0.0, 1.0);
}

void main()
{
    vec4 worldPos = u_invViewProj * vec4(v_uv * 2.0 - 1.0, texture(u_depthTexture, v_uv).r * 2.0 - 1.0, 1.0);
    vec3 position = worldPos.xyz / worldPos.w;
    float depth   = length(position);
    vec3 rayDir   = position / depth;

    float top    = u_cloudBottom + kThickness;
    float tEnter = 0.0;
    float tExit  = u_maxDistance;
    if (abs(rayDir.y) > 0.0001)
    {
        float t0 = u_cloudBottom / rayDir.y;
        float t1 = top / rayDir.y;
        tEnter   = max(min(t0, t1), 0.0);
        tExit    = min(max(t0, t1), u_maxDistance);
    }
    else if (u_cloudBottom > 0.0 || top < 0.0)
    {
        tExit = -1.0;
    }

    tExit = min(tExit, depth);
    if (tExit <= tEnter)
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 lightDir   = u_sunDirection.y >= 0.0 ? u_sunDirection : -u_sunDirection;
    float lightGain = u_sunDirection.y >= 0.0 ? 1.0 : 0.25;
    float sunset    = 1.0 - clamp(abs(u_sunDirection.y) / 0.35, 0.0, 1.0);
    vec3 direct     = u_cloudColor * mix(vec3(1.0), vec3(1.35, 0.85, 0.6), sunset * float(u_sunDirection.y >= 0.0)) * lightGain;
    vec3 ambient    = u_cloudColor * 0.55;
    float cosTheta  = dot(rayDir, lightDir);
    float g         = 0.55;
    float phase     = (1.0 - g * g) / (4.0 * 3.14159265 * pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5)) * 4.0 * 3.14159265 * 0.5 + 0.5;

    float stepSize      = (tExit - tEnter) / u_steps;
    float dither        = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
    float transmittance = 1.0;
    vec3 scattered      = vec3(0.0);
    float hitDistance   = tExit;
    for (int i = 0; i < int(u_steps) && transmittance > 0.02; i++)
    {
        float t = tEnter + (float(i) + dither) * stepSize;
        vec3 p  = rayDir * t;
        float d = Density(p);
        if (d <= 0.0)
        {
            continue;
        }

        hitDistance      = min(hitDistance, t);
        float lightDepth = 0.0;
        for (int j = 1; j <= 3; j++)
        {
            lightDepth += Density(p + lightDir * float(j) * 2.0) * 2.0;
        }

        float lightT   = exp(-kExtinction * lightDepth);
        float powder   = 1.0 - exp(-kExtinction * 2.0 * d * 4.0);
        float flash    = u_flash.w * clamp(1.0 - length(p - u_flash.xyz) / 96.0, 0.0, 1.0);
        vec3 luminance = ambient + direct * lightT * phase * mix(1.0, powder, 0.5) + vec3(0.85, 0.88, 1.0) * flash * flash;
        float stepT    = exp(-kExtinction * d * stepSize);
        scattered += transmittance * (1.0 - stepT) * luminance;
        transmittance *= stepT;
    }

    float fade = 1.0 - smoothstep(u_maxDistance * 0.7, u_maxDistance, hitDistance);
    scattered = mix(scattered, u_fogColor * (1.0 - transmittance), DistanceFog(hitDistance));
    // The underwater fog pass runs before the clouds, so we fog them the same way here
    if (u_underwaterFog > 0.5)
    {
        float underwaterFog = max(1.0 - exp(-u_underwaterFogDensity * hitDistance), u_underwaterFogMinimum);
        scattered           = mix(scattered, u_underwaterFogColor * (1.0 - transmittance), clamp(underwaterFog, 0.0, 1.0));
    }

    FragColor = vec4(scattered * fade, mix(1.0, transmittance, fade));
}
