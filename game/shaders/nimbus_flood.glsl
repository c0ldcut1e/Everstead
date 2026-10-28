// We filter each slice in hardware and blend two slices ourselves, clamping inside the slice so it never reads its neighbor in the atlas
vec4 NimbusFloodSample(vec3 position)
{
    vec3 cell    = clamp(position - u_nimbusFloodOrigin, 0.5, 127.5);
    float z      = cell.z - 0.5;
    int slice    = int(z);
    int next     = min(slice + 1, 127);
    vec2 below   = vec2(ivec2(slice & 15, slice >> 4) * 128) + cell.xy;
    vec2 above   = vec2(ivec2(next & 15, next >> 4) * 128) + cell.xy;
    vec4 sample0 = textureLod(u_nimbusFlood, below / vec2(2048.0, 1024.0), 0.0);
    vec4 sample1 = textureLod(u_nimbusFlood, above / vec2(2048.0, 1024.0), 0.0);
    // The flood stores light squared and scaled down by 4 so bright lights stay inside half float range
    return mix(sample0, sample1, fract(z)) * 4.0;
}

float NimbusFloodFade(vec3 position)
{
    vec3 offset = abs(position - u_nimbusFloodOrigin - 64.0);
    return clamp((max(max(offset.x, offset.y), offset.z) - 48.0) / 16.0, 0.0, 1.0);
}
