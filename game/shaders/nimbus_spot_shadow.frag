#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform usampler2D u_nimbusGrid;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_nimbusLightCount;
    vec4 u_nimbusLightPositions[32];
    vec4 u_nimbusLightColors[32];
    vec4 u_nimbusLightDirections[32];
    vec4 u_nimbusLightParams[32];
    vec3 u_nimbusGridOrigin;
    float u_nimbusOcclusion;
    vec3 u_nimbusGridWrap;
};

layout(location = 0) out vec4 FragColor;

#include "nimbus_trace.glsl"
#include "nimbus_spot_map.glsl"

void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    int index   = (pixel.x >> 8) + (pixel.y >> 8) * 8;
    if (index >= int(u_nimbusLightCount) || abs(u_nimbusLightColors[index].w - 1.0) > 0.5 || u_nimbusLightParams[index].y < 0.5)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec2 local         = (vec2(pixel & 255) + 0.5) / 128.0 - 1.0;
    vec3 dir           = normalize(NimbusSpotBasis(index) * vec3(local * NimbusSpotSpread(index), 1.0));
    vec4 lightPosition = u_nimbusLightPositions[index];
    float hit = lightPosition.w;
    if (u_nimbusLightParams[index].w > 0.5)
    {
        NimbusTraceHit(lightPosition.xyz, lightPosition.xyz + dir * lightPosition.w, hit);
    }

    FragColor = vec4(hit);
}
