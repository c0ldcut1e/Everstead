vec2 NimbusSunTexel(int channel, ivec3 cell)
{
    if (any(lessThan(cell, ivec3(0))) || any(greaterThanEqual(cell, ivec3(256))))
    {
        return vec2(1.0);
    }

    ivec3 wrapped = (cell + ivec3(u_nimbusGridWrap)) & 255;
    ivec3 section = wrapped >> 4;
    ivec3 inner   = wrapped & 15;
    int slot      = section.x + section.y * 16 + section.z * 256;
    vec4 texel =
            texelFetch(u_nimbusSunCache, ivec2((slot & 63) * 64 + (inner.z & 3) * 16 + inner.x, (slot >> 6) * 64 + (inner.z >> 2) * 16 + inner.y), 0);
    return vec2(texel[channel], texel.a);
}

// Solid cells get no weight, so a lit face never blends with the dark inside of the block it sits on
float NimbusSunVisibility(int index, vec3 position)
{
    int channel = int(u_nimbusLightParams[index].z);
    if (channel < 0)
    {
        return 1.0;
    }

    vec3 cell  = position - u_nimbusGridOrigin - 0.5;
    vec3 edge  = min(cell, 255.0 - cell);
    float fade = smoothstep(0.0, 32.0, min(min(edge.x, edge.y), edge.z));
    ivec3 base = ivec3(floor(cell));
    vec3 f     = cell - vec3(base);
    vec2 sum   = vec2(0.0);
    for (int i = 0; i < 8; i++)
    {
        ivec3 corner = ivec3(i & 1, (i >> 1) & 1, i >> 2);
        vec3 weight  = mix(1.0 - f, f, vec3(corner));
        sum += NimbusSunTexel(channel, base + corner) * weight.x * weight.y * weight.z;
    }

    return sum.y > 0.0001 ? mix(1.0, sum.x / sum.y, fade) : 1.0;
}
