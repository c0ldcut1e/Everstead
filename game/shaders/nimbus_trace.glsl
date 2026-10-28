// Each grid row holds one section as 128 solid words, 128 water words, two brick words and a nonempty word
uint NimbusWord(int slot, int word)
{
    return texelFetch(u_nimbusGrid, ivec2(word, slot), 0).r;
}

// Hit is how far the ray got before a block stopped it, we skip empty sections and empty 4x4x4 bricks in one step, so long rays through air stay cheap
float NimbusTraceHit(vec3 from, vec3 to, out float hit)
{
    vec3 delta = to - from;
    float len  = length(delta);
    hit        = len;
    if (u_nimbusOcclusion < 0.5)
    {
        return 1.0;
    }

    from -= u_nimbusGridOrigin;
    to -= u_nimbusGridOrigin;
    if (len < 0.001)
    {
        return 1.0;
    }

    vec3 dir            = delta / len;
    dir                 = mix(dir, vec3(0.000001), lessThan(abs(dir), vec3(0.000001)));
    vec3 invDir         = 1.0 / dir;
    vec3 exitSide       = vec3(greaterThan(dir, vec3(0.0)));
    ivec3 startVox      = ivec3(floor(from));
    ivec3 endVox        = ivec3(floor(to));
    ivec3 wrap          = ivec3(u_nimbusGridWrap);
    float t             = 0.0;
    float transmittance = 1.0;
    // A ray never crosses more voxels than this, so it reaches its end instead of running out of steps and reading as lit
    int steps   = int(abs(delta.x) + abs(delta.y) + abs(delta.z)) + 4;
    ivec3 voxel = startVox;
    for (int i = 0; i < steps; i++)
    {
        if (t >= len || voxel == endVox || any(lessThan(voxel, ivec3(0))) || any(greaterThanEqual(voxel, ivec3(256))))
        {
            return transmittance;
        }

        ivec3 wrapped = (voxel + wrap) & 255;
        ivec3 section = wrapped >> 4;
        int slot      = section.x + section.y * 16 + section.z * 256;
        int size      = 16;
        if (NimbusWord(slot, 258) != 0u)
        {
            ivec3 inner = wrapped & 15;
            int brick   = (inner.x >> 2) + (inner.z >> 2) * 4 + (inner.y >> 2) * 16;
            size        = 4;
            if (((NimbusWord(slot, 256 + (brick >> 5)) >> uint(brick & 31)) & 1u) != 0u)
            {
                size = 1;
                if (voxel != startVox)
                {
                    int local = (inner.x & 3) + (inner.z & 3) * 4 + (inner.y & 3) * 16;
                    int word  = brick * 2 + (local >> 5);
                    uint bit  = 1u << uint(local & 31);
                    if ((NimbusWord(slot, word) & bit) != 0u)
                    {
                        hit = t;
                        return 0.0;
                    }

                    if ((NimbusWord(slot, 128 + word) & bit) != 0u)
                    {
                        transmittance *= 0.8;
                        if (transmittance < 0.02)
                        {
                            hit = t;
                            return 0.0;
                        }
                    }
                }
            }
        }

        ivec3 cellMin = voxel & ~(size - 1);
        vec3 exitT    = (vec3(cellMin) + exitSide * float(size) - from) * invDir;
        t             = min(min(exitT.x, exitT.y), exitT.z);
        // We step one axis at a time so a ray through a block edge visits a side voxel instead of slipping diagonally between two blocks
        ivec3 next = clamp(ivec3(floor(from + dir * t)), cellMin, cellMin + size - 1);
        if (t == exitT.x)
        {
            next.x = dir.x > 0.0 ? cellMin.x + size : cellMin.x - 1;
        }
        else if (t == exitT.y)
        {
            next.y = dir.y > 0.0 ? cellMin.y + size : cellMin.y - 1;
        }
        else
        {
            next.z = dir.z > 0.0 ? cellMin.z + size : cellMin.z - 1;
        }

        voxel = next;
    }

    return transmittance;
}

float NimbusTrace(vec3 from, vec3 to)
{
    float hit;
    return NimbusTraceHit(from, to, hit);
}

float NimbusTint(vec3 position)
{
    if (u_nimbusOcclusion < 0.5)
    {
        return 1.0;
    }

    ivec3 voxel = ivec3(floor(position - u_nimbusGridOrigin));
    if (any(lessThan(voxel, ivec3(0))) || any(greaterThanEqual(voxel, ivec3(256))))
    {
        return 1.0;
    }

    ivec3 wrapped = (voxel + ivec3(u_nimbusGridWrap)) & 255;
    ivec3 section = wrapped >> 4;
    ivec3 inner   = wrapped & 15;
    int slot      = section.x + section.y * 16 + section.z * 256;
    if (NimbusWord(slot, 258) == 0u)
    {
        return 1.0;
    }

    int local = (inner.x & 3) + (inner.z & 3) * 4 + (inner.y & 3) * 16;
    int word  = ((inner.x >> 2) + (inner.z >> 2) * 4 + (inner.y >> 2) * 16) * 2 + (local >> 5);
    uint bit  = 1u << uint(local & 31);
    if ((NimbusWord(slot, word) & bit) != 0u)
    {
        return 0.0;
    }

    return (NimbusWord(slot, 128 + word) & bit) != 0u ? 0.8 : 1.0;
}

bool NimbusSolid(vec3 position)
{
    return NimbusTint(position) == 0.0;
}
