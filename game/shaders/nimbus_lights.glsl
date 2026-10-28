// This is the rate the flood falls off at in open air, so traced and flooded light match
float NimbusFalloff(float dist, float radius)
{
    float window  = clamp(1.0 - pow(dist / max(radius, 0.1), 4.0), 0.0, 1.0);
    float clamped = max(dist, 1.0);
    return sqrt(exp(-u_nimbusFloodDecay * (clamped - 1.0)) / clamped) * window * window;
}

float NimbusAttenuation(int index, vec3 position, out vec3 toLight)
{
    vec4 lightPosition = u_nimbusLightPositions[index];
    vec4 lightDir      = u_nimbusLightDirections[index];
    if (u_nimbusLightColors[index].w > 1.5)
    {
        toLight = -lightDir.xyz;
        return 1.0;
    }

    vec3 delta        = lightPosition.xyz - position;
    float dist        = length(delta);
    float attenuation = NimbusFalloff(dist, lightPosition.w);
    toLight           = delta / max(dist, 0.0001);
    if (u_nimbusLightColors[index].w > 0.5)
    {
        attenuation *= smoothstep(lightDir.w, mix(lightDir.w, 1.0, 0.2), dot(-toLight, lightDir.xyz));
    }

    return attenuation;
}
