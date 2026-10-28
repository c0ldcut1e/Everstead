// Each spot light gets a square of the shadow map atlas spanning its cone, indexed by the light's slot
mat3 NimbusSpotBasis(int index)
{
    vec3 forward = u_nimbusLightDirections[index].xyz;
    vec3 right   = normalize(cross(forward, abs(forward.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0)));
    return mat3(right, cross(right, forward), forward);
}

// We clamp wide cones because the square projection stretches to infinity at 90 degrees
float NimbusSpotSpread(int index)
{
    float cutoff = max(u_nimbusLightDirections[index].w, 0.1);
    return sqrt(1.0 - cutoff * cutoff) / cutoff;
}
