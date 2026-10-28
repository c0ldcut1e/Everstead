#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_froxelScatter;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    vec3 u_froxelGrid;
    float u_maxDistance;
};

layout(location = 0) out vec4 FragColor;

#include "nimbus_froxel.glsl"

// Each froxel holds the light and transmittance from the camera to the far side of its slice
void main()
{
    ivec3 froxel        = FroxelAt(ivec2(gl_FragCoord.xy));
    vec3 inscatter      = vec3(0.0);
    float transmittance = 1.0;
    for (int slice = 0; slice <= froxel.z; slice++)
    {
        vec4 medium      = texelFetch(u_froxelScatter, FroxelTexel(ivec3(froxel.xy, slice)), 0);
        float thickness  = FroxelDepth(float(slice + 1)) - FroxelDepth(float(slice));
        float extinction = medium.a * thickness;
        // We integrate the scattering across the slice analytically so thick fog doesn't gain energy from long slices
        inscatter += transmittance * (extinction > 0.00001 ? medium.rgb * (1.0 - exp(-extinction)) / medium.a : medium.rgb * thickness);
        transmittance *= exp(-extinction);
    }

    FragColor = vec4(inscatter, transmittance);
}
