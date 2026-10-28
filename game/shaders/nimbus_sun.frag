#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform usampler2D u_nimbusGrid;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    vec4 u_sunDirections[3];
    float u_sunCount;
    vec3 u_sunSection;
    vec2 u_sunTile;
    float u_cacheSamples;
    float u_nimbusOcclusion;
    vec3 u_nimbusGridOrigin;
    vec3 u_nimbusGridWrap;
};

layout(location = 0) out vec4 FragColor;

#include "nimbus_trace.glsl"

// We spread the rays over a disk the size of the sun so the penumbra grows with distance from the blocker
float SoftTrace(vec3 from, vec3 target, float radius, int samples)
{
    if (samples <= 1 || radius <= 0.0)
    {
        return NimbusTrace(from, target);
    }

    vec3 toLight     = normalize(target - from + vec3(0.0, 0.0001, 0.0));
    vec3 tangent     = normalize(cross(toLight, abs(toLight.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    vec3 bitangent   = cross(toLight, tangent);
    float visibility = 0.0;
    float weight     = 0.0;
    for (int i = 0; i < samples; i++)
    {
        float angle  = float(i) * 2.3999632;
        float offset = sqrt((float(i) + 0.5) / float(samples)) * radius;
        vec3 point   = target + (tangent * cos(angle) + bitangent * sin(angle)) * offset;
        // Disk points behind a wall the light touches would let rays go around the wall, so only points the light center sees count
        if (!NimbusSolid(point) && NimbusTrace(target, point) > 0.0)
        {
            visibility += NimbusTrace(from, point);
            weight += 1.0;
        }
    }

    return weight > 0.0 ? visibility / weight : NimbusTrace(from, target);
}

// Each section gets a 64x64 tile of 16 slices, rgb holds up to three directional lights and alpha marks cells that are not solid
void main()
{
    ivec2 pixel = ivec2(gl_FragCoord.xy) - ivec2(u_sunTile);
    vec3 p      = u_sunSection + vec3(pixel.x & 15, pixel.y & 15, (pixel.x >> 4) + (pixel.y >> 4) * 4) + 0.5;
    if (NimbusSolid(p))
    {
        FragColor = vec4(0.0);
        return;
    }

    vec3 visibility = vec3(1.0);
    for (int i = 0; i < int(u_sunCount); i++)
    {
        vec4 sun      = u_sunDirections[i];
        visibility[i] = SoftTrace(p, p + sun.xyz * 256.0, sun.w * 256.0, int(u_cacheSamples));
    }

    FragColor = vec4(visibility, 1.0);
}
