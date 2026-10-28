#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform usampler2D u_nimbusGrid;
layout(set = 1, binding = 1) uniform sampler2D u_floodPrevious;
layout(set = 1, binding = 2) uniform sampler2D u_floodEmissionTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    vec4 u_nimbusLightPositions[32];
    vec4 u_nimbusLightColors[32];
    vec4 u_nimbusLightParams[32];
    vec4 u_floodBouncePositions[64];
    vec4 u_floodBounceColors[64];
    vec3 u_floodShift;
    float u_nimbusLightCount;
    vec3 u_nimbusGridOrigin;
    float u_nimbusOcclusion;
    vec3 u_nimbusGridWrap;
    float u_nimbusFloodDecay;
    float u_nimbusBounce;
    float u_floodBounceCount;
    vec3 u_floodEmissionOffset;
    float u_floodEmission;
};

layout(location = 0) out vec4 FragColor;

#include "nimbus_trace.glsl"

vec4 FloodTexel(ivec3 cell)
{
    cell = clamp(cell, ivec3(0), ivec3(127));
    return texelFetch(u_floodPrevious, ivec2((cell.z & 15) * 128 + cell.x, (cell.z >> 4) * 128 + cell.y), 0);
}

// The volume is 128 slices of 128x128 cells laid out 16 by 8, rgb holds point lights squared
void main()
{
    ivec2 pixel  = ivec2(gl_FragCoord.xy);
    ivec3 cell   = ivec3(pixel & 127, (pixel.x >> 7) + (pixel.y >> 7) * 16);
    vec3 center  = vec3(cell) + 0.5;
    float tint   = NimbusTint(center);
    ivec3 source = cell + ivec3(u_floodShift);
    vec4 light   = vec4(0.0);
    if (tint > 0.0 && all(greaterThanEqual(source, ivec3(0))) && all(lessThan(source, ivec3(128))))
    {
        vec4 previous = FloodTexel(source);
        vec4 gather   = previous * 1.05;
        for (int face = 0; face < 6; face++)
        {
            ivec3 offset     = ivec3(0);
            offset[face / 2] = (face & 1) * 2 - 1;
            // A solid neighbor reflects part of this cell's light back instead of absorbing it, which is what lights up walls around a lamp
            gather += NimbusTint(center + vec3(offset)) > 0.0 ? FloodTexel(source + offset) : previous * u_nimbusBounce;
        }

        // This divisor makes the flood fall off at the same decay the shaders use for traced light
        light = gather / (5.05 + 2.0 * cosh(u_nimbusFloodDecay)) * tint * tint;
    }

    for (int i = 0; i < int(u_nimbusLightCount) && tint > 0.0; i++)
    {
        vec3 position = u_nimbusLightPositions[i].xyz;
        vec3 offset   = abs(center - position);
        if (u_nimbusLightColors[i].w > 0.5 || u_nimbusLightParams[i].w < 0.5 || any(greaterThanEqual(offset, vec3(1.0))))
        {
            continue;
        }

        // We split each light over the open cells around it, so a light against a wall keeps its full brightness
        vec3 base  = floor(position - 0.5) + 0.5;
        float open = 0.0;
        for (int c = 0; c < 8; c++)
        {
            vec3 corner = base + vec3(c & 1, (c >> 1) & 1, c >> 2);
            vec3 weight = max(1.0 - abs(corner - position), 0.0);
            open += NimbusTint(corner) > 0.0 ? weight.x * weight.y * weight.z : 0.0;
        }

        vec3 rgb    = u_nimbusLightColors[i].rgb;
        vec3 weight = 1.0 - offset;
        // The flood settles to about this fraction of its seed one cell away and is stored at a quarter, so that cell holds the light's color
        float seed = weight.x * weight.y * weight.z / open * 0.25 / (0.61 * exp(-0.87 * u_nimbusFloodDecay));
        light.rgb += rgb * rgb * seed;
    }

    if (u_floodEmission > 0.5 && tint > 0.0)
    {
        // Emitting blocks are usually solid, so an open cell takes the brightest emitter it touches as its seed
        vec3 emitted = vec3(0.0);
        for (int face = 0; face < 7; face++)
        {
            ivec3 offset = ivec3(0);
            if (face < 6)
            {
                offset[face / 2] = (face & 1) * 2 - 1;
            }

            ivec3 emitCell = cell + offset + ivec3(u_floodEmissionOffset);
            if (all(greaterThanEqual(emitCell, ivec3(0))) && all(lessThan(emitCell, ivec3(128))))
            {
                emitted = max(emitted, texelFetch(u_floodEmissionTexture, ivec2((emitCell.z & 15) * 128 + emitCell.x, (emitCell.z >> 4) * 128 + emitCell.y), 0).rgb);
            }
        }

        light.rgb += emitted * emitted * (0.25 / (0.61 * exp(-0.87 * u_nimbusFloodDecay)));
    }

    for (int i = 0; i < int(u_floodBounceCount) && tint > 0.0; i++)
    {
        vec3 weight = 1.0 - abs(center - u_floodBouncePositions[i].xyz);
        if (all(greaterThan(weight, vec3(0.0))))
        {
            vec3 rgb = u_floodBounceColors[i].rgb;
            light.rgb += rgb * rgb * (weight.x * weight.y * weight.z * 0.25 / (0.61 * exp(-0.87 * u_nimbusFloodDecay)));
        }
    }

    FragColor = light;
}
