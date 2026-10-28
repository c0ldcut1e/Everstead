// We only look past the cached cells ahead of the face where they disagree, since that is where sun shadow edges fall
float NimbusSunShadow(int index, vec3 origin, vec3 toLight)
{
    int channel = int(u_nimbusLightParams[index].z);
    if (channel < 0)
    {
        return 1.0;
    }

    vec3 ahead = origin + toLight * 2.0;
    // The trace skips its end voxel, so the tint catches an ahead point that landed inside a block
    float near = NimbusTrace(origin, ahead) * NimbusTint(ahead);
    ivec3 base = ivec3(floor(ahead - u_nimbusGridOrigin - 0.5));
    // Slots at the grid edge can still hold the cache of the section that wrapped away
    if (near <= 0.0 || any(lessThan(base, ivec3(16))) || any(greaterThanEqual(base, ivec3(239))))
    {
        return near > 0.0 ? NimbusTrace(origin, origin + toLight * 256.0) : 0.0;
    }

    vec2 range = vec2(1.0, 0.0);
    for (int i = 0; i < 8; i++)
    {
        vec2 texel = NimbusSunTexel(channel, base + ivec3(i & 1, (i >> 1) & 1, i >> 2));
        if (texel.y > 0.5)
        {
            range = vec2(min(range.x, texel.x), max(range.y, texel.x));
        }
    }

    if (range.x <= range.y && range.y - range.x < 0.02)
    {
        return near * (range.x + range.y) * 0.5;
    }

    return u_nimbusMode < 0.5 ? near * NimbusSunVisibility(index, ahead) : NimbusTrace(origin, origin + toLight * 256.0);
}

// We compress bright light by its peak channel so it rolls off toward full brightness without clipping channels to white
vec3 NimbusShoulder(vec3 light)
{
    float peak = max(max(light.r, light.g), light.b);
    if (peak <= 0.7)
    {
        return light;
    }

    return light * ((0.7 + 0.3 * (1.0 - exp((0.7 - peak) / 0.3))) / peak);
}

vec4 NimbusFloodSurfaceSample(vec3 position)
{
    vec3 cell  = clamp(position - u_nimbusFloodOrigin, 0.5, 127.5) - 0.5;
    ivec3 base = ivec3(cell);
    vec3 f     = cell - vec3(base);
    int home   = int(f.x >= 0.5) | (int(f.y >= 0.5) << 1) | (int(f.z >= 0.5) << 2);
    vec4 texels[8];
    uint open = 0u;
    for (int c = 0; c < 8; c++)
    {
        ivec3 corner = min(base + ivec3(c & 1, (c >> 1) & 1, c >> 2), ivec3(127));
        texels[c]    = texelFetch(u_nimbusFlood, ivec2((corner.z & 15) * 128 + corner.x, (corner.z >> 4) * 128 + corner.y), 0);
        open |= NimbusTint(u_nimbusFloodOrigin + vec3(corner) + 0.5) > 0.0 ? 1u << uint(c) : 0u;
    }

    // Sides of a lava pool can sample deep enough that every corner is solid, so we take the brightest open cell next to the sample
    if (open == 0u)
    {
        ivec3 inside = ivec3(floor(clamp(position - u_nimbusFloodOrigin, 0.5, 127.5)));
        vec4 brightest = vec4(0.0);
        for (int i = 0; i < 6; i++)
        {
            ivec3 neighbor = clamp(inside + ivec3(i == 0 ? 1 : i == 1 ? -1 : 0, i == 2 ? 1 : i == 3 ? -1 : 0, i == 4 ? 1 : i == 5 ? -1 : 0), ivec3(0), ivec3(127));
            vec4 texel     = texelFetch(u_nimbusFlood, ivec2((neighbor.z & 15) * 128 + neighbor.x, (neighbor.z >> 4) * 128 + neighbor.y), 0);
            if (NimbusTint(u_nimbusFloodOrigin + vec3(neighbor) + 0.5) > 0.0 && dot(texel.rgb, vec3(1.0)) > dot(brightest.rgb, vec3(1.0)))
            {
                brightest = texel;
            }
        }

        return brightest * 4.0;
    }

    // Faces against lava or poking into a wall land in a solid cell, which holds no light, so we start from the heaviest open corner
    if ((open & (1u << uint(home))) == 0u)
    {
        float best = 0.0;
        for (int c = 0; c < 8; c++)
        {
            vec3 w       = mix(1.0 - f, f, vec3(c & 1, (c >> 1) & 1, c >> 2));
            float weight = w.x * w.y * w.z;
            if (((open >> uint(c)) & 1u) != 0u && weight > best)
            {
                best = weight;
                home = c;
            }
        }
    }

    // Open cells that only touch the home cell across a solid edge or corner hold light this face can't see, so we swap in the home cell
    uint reach = 1u << uint(home);
    for (int i = 0; i < 3; i++)
    {
        uint spread = 0u;
        for (int c = 0; c < 8; c++)
        {
            spread |= ((reach >> uint(c)) & 1u) != 0u ? (1u << uint(c ^ 1)) | (1u << uint(c ^ 2)) | (1u << uint(c ^ 4)) : 0u;
        }

        reach |= spread & open;
    }

    for (int c = 0; c < 8; c++)
    {
        texels[c] = (open & ~reach & (1u << uint(c))) != 0u ? texels[home] : texels[c];
    }

    vec4 x0 = mix(mix(texels[0], texels[1], f.x), mix(texels[2], texels[3], f.x), f.y);
    vec4 x1 = mix(mix(texels[4], texels[5], f.x), mix(texels[6], texels[7], f.x), f.y);
    return mix(x0, x1, f.z) * 4.0;
}

vec3 NimbusSurfaceLight(vec3 position, vec3 normal, vec3 baseLight)
{
    bool floodfill  = u_nimbusMode < 0.5;
    // Faces inside their block like lantern sides and fluid tops can land half a step into a solid block, and fluid sides against a wall face straight into one, so we try the face's own cell from both sides and then a full step out
    vec3 floodPoint = position + normal * 0.5;
    if (NimbusTint(floodPoint) <= 0.0)
    {
        floodPoint = position + normal * 0.01;
        if (NimbusTint(floodPoint) <= 0.0)
        {
            floodPoint = NimbusTint(position - normal * 0.25) > 0.0 ? position - normal * 0.25 : position + normal;
        }
    }

    float fade      = NimbusFloodFade(floodPoint);
    vec3 total      = vec3(0.0);
    if (u_nimbusFloodLights > 0.5 && fade < 1.0)
    {
        total = sqrt(NimbusFloodSurfaceSample(floodPoint).rgb) * ((1.0 - fade) * (floodfill ? 1.0 : u_nimbusBounce));
        if (u_nimbusLighting > 0.5)
        {
            // We only warm up near white block light because the falloff crushes the blue out of colored lamps
            total       = NimbusShoulder(total);
            float peak  = max(max(total.r, total.g), total.b);
            float white = 1.0 - smoothstep(0.35, 0.6, (peak - min(min(total.r, total.g), total.b)) / max(peak, 0.0001));
            // The flood is much brighter than the light level ramp, so we warm by the ramped peak or near light would stay white
            float ramp  = BrightnessRamp(peak);
            total.g     = mix(total.g, total.g * ((ramp * 0.6 + 0.4) * 0.6 + 0.4), white);
            total.b     = mix(total.b, total.b * (ramp * ramp * 0.6 + 0.4), white);
            // Block light adds on top of skylight instead of taking the brighter one, so we only add what the surface is missing plus a faint tint
            // Flicker scales what's left after the skylight so a small wobble doesn't swing a mostly lit surface
            total = max(total - clamp(baseLight, 0.0, 1.0), total * 0.15) * u_blockLightScale;
        }
    }

    vec3 origin = position + normal * 0.01;
    uint mask   = texelFetch(u_nimbusTiles, ivec2(gl_FragCoord.xy / u_nimbusTilePixels), 0).r;
    for (int i = 0; i < int(u_nimbusLightCount) && (mask >> uint(i)) != 0u; i++)
    {
        if (((mask >> uint(i)) & 1u) == 0u)
        {
            continue;
        }

        vec3 toLight;
        float type    = u_nimbusLightColors[i].w;
        bool occluded = u_nimbusLightParams[i].w > 0.5;
        if (floodfill && type < 0.5 && occluded)
        {
            // Point lights past the flood volume go unshadowed instead of disappearing
            if (fade > 0.0)
            {
                total += u_nimbusLightColors[i].rgb * NimbusAttenuation(i, floodPoint, toLight) * fade;
            }

            continue;
        }

        float lit = NimbusAttenuation(i, position, toLight) * max(dot(normal, toLight), 0.0);
        if (lit > 0.001)
        {
            float shadow = !occluded ? 1.0 : type > 1.5 ? NimbusSunShadow(i, origin, toLight) : NimbusTrace(origin, u_nimbusLightPositions[i].xyz);
            total += u_nimbusLightColors[i].rgb * lit * shadow;
        }
    }

    return u_nimbusLighting > 0.5 ? NimbusShoulder(total) : total;
}
