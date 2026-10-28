float Phase(float cosTheta)
{
    float g     = u_anisotropy;
    float denom = 1.0 + g * g - 2.0 * g * cosTheta;
    return (1.0 - g * g) / (12.566371 * pow(max(denom, 0.0001), 1.5));
}

#include "world_fog.glsl"

// The world fog pass already ran, so we fade our own fog out the same way instead of drawing it over distant fogged terrain
float WorldFog(float viewDistance)
{
    if (u_underwaterFog > 0.5)
    {
        return max(1.0 - exp(-u_underwaterFogDensity * viewDistance), u_underwaterFogMinimum);
    }

    return DistanceFog(viewDistance);
}

// The flood holds the light that got here around blocks, so its ratio to the unblocked light is how visible the light is
float FloodVisibility(float flooded, float expected, float fade)
{
    if (u_nimbusOcclusion < 0.5 || expected < 0.000001)
    {
        return 1.0;
    }

    return mix(sqrt(clamp(flooded / expected, 0.0, 1.0)), 1.0, fade);
}

// Point lights only scatter through air here because the flood already carries their light around blocks
vec3 NimbusPointScatter(vec3 p, vec3 rayDir, bool floodScatter)
{
    vec3 flooded   = vec3(0.0);
    vec3 open      = vec3(0.0);
    float expected = 0.0;
    for (int l = 0; l < int(u_nimbusLightCount); l++)
    {
        vec4 lightPosition = u_nimbusLightPositions[l];
        if (u_nimbusLightColors[l].w > 0.5 || dot(p - lightPosition.xyz, p - lightPosition.xyz) >= lightPosition.w * lightPosition.w)
        {
            continue;
        }

        vec3 toLight;
        vec3 color = u_nimbusLightColors[l].rgb * NimbusAttenuation(l, p, toLight);
        if (u_nimbusLightParams[l].w < 0.5 || floodScatter)
        {
            open += u_nimbusLightParams[l].y > 0.5 && u_nimbusLightParams[l].w < 0.5 ? color * Phase(dot(rayDir, toLight)) : vec3(0.0);
            continue;
        }

        expected += dot(color, color);
        if (u_nimbusLightParams[l].y > 0.5)
        {
            flooded += color * Phase(dot(rayDir, toLight));
        }
    }

    if (floodScatter)
    {
        // The flood mixes every occluded light without a direction, so we scatter it evenly
        return (open + sqrt(NimbusFloodSample(p).rgb) * ((1.0 - NimbusFloodFade(p)) * 0.0795775)) * (u_airDensity + u_nimbusFog * 1.2 / u_fogRange.y);
    }

    if (expected <= 0.0)
    {
        return open * (u_airDensity + u_nimbusFog * 1.2 / u_fogRange.y);
    }

    vec4 flood = NimbusFloodSample(p);
    return (open + flooded * FloodVisibility(flood.r + flood.g + flood.b, expected, NimbusFloodFade(p))) * (u_airDensity + u_nimbusFog * 1.2 / u_fogRange.y);
}
