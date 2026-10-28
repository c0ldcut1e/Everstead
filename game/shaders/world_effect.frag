#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_depthTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invViewProj;
    vec4 u_effects[16];
    vec4 u_effectStates[16];
    vec4 u_flash;
    float u_effectCount;
    float u_time;
    float u_frame;
};

layout(location = 0) out vec4 FragColor;

// These match WorldEffects::eWORLD_EFFECT_TYPE
const int kEffectRiftShockwave       = 0;
const int kEffectRiftDetonation      = 1;
const int kEffectRiftImplosion       = 2;
const int kEffectExperimentTesseract = 3;
const int kEffectExperimentBurst     = 4;
const int kEffectExperimentFinale    = 5;
const int kEffectSingularityCharge   = 6;
const int kEffectSingularityBlast    = 7;
const int kEffectNovaCharge          = 8;
const int kEffectNovaBlast           = 9;
const int kEffectTempestCharge       = 10;
const int kEffectTempestBlast        = 11;
const int kEffectPrismCharge         = 12;
const int kEffectPrismBlast          = 13;
const int kEffectHellgateCharge      = 14;
const int kEffectHellgateBlast       = 15;
const int kEffectPhaseGlitch         = 16;
const int kEffectFracturePulse       = 17;
const int kEffectFractureCrack       = 18;
const int kEffectFractureTear        = 19;

const int kShockwaveSteps   = 36;
const int kDetonationSteps  = 64;
const int kDetonationSpikes = 12;

#include "void.glsl"

float ShockwaveRadius(float radius, float progress)
{
    float grow = 1.0 - pow(1.0 - min(progress / 0.12, 1.0), 3.0);
    return radius * 1.6 * grow * (1.0 - smoothstep(0.2, 0.54, progress));
}

// This is the signed distance to the torn sphere, and shards whose hash falls under the break amount drop out as it collapses
float Shockwave(vec3 p, float shellRadius, float seed, float progress)
{
    vec3 direction = p / max(length(p), 0.0001);
    float d        = length(p) - shellRadius * (1.0 + (Noise(direction * 4.0 + vec3(seed, u_time * 1.5, 0.0)) - 0.5) * 0.35);
    float shard    = Hash(ivec3(floor(direction * 5.0 + seed)));
    return shard < smoothstep(0.35, 0.7, progress) ? max(d, 0.3) : d;
}

vec4 RiftShockwave(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed, float jitter)
{
    float energy      = 1.0 - progress;
    float burst       = exp(-progress * 14.0);
    float shellRadius = ShockwaveRadius(radius, progress);
    vec3 color        = vec3(0.0);

    // The ring is measured from the ray's closest point to the center, so it always faces the camera like a halo
    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT)
    {
        float h     = length(toCenter - ray * closest);
        float ringR = radius * 4.5 * sqrt(progress);
        color += (kRimColor * exp(-abs(h - ringR) * 1.2) * 1.8 + kGlowColor * exp(-abs(h - ringR) * 0.25) * 0.4) * energy * energy;
    }

    vec3 o     = ro - center;
    float b    = dot(o, ray);
    float disc = b * b - dot(o, o) + (shellRadius * 1.3 + 1.0) * (shellRadius * 1.3 + 1.0);
    if (disc <= 0.0)
    {
        return vec4(color, 0.0);
    }

    float root    = sqrt(disc);
    float t0      = max(-b - root, 0.0);
    float t1      = min(-b + root, sceneT);
    float opacity = 0.0;
    float flicker = 0.7 + 0.3 * Noise(vec3(u_time * 11.0, seed, 3.0));
    float t       = t0 + jitter * 0.1;
    for (int s = 0; s < kShockwaveSteps && t < t1; s++)
    {
        vec3 p     = ro + ray * t - center;
        float dist = Shockwave(p, shellRadius, seed, progress);
        if (dist < 0.0)
        {
            color += Void(p / max(shellRadius, 0.5), ray, seed, u_time) * (1.0 + burst * 3.0);
            opacity = 1.0;
            break;
        }

        float dt   = clamp(dist * 0.6, 0.04, 1.5);
        float arc  = dist < 1.5 ? abs(Noise(p * 1.3 + vec3(seed, u_time * 3.0, 0.0)) - 0.5) : 1.0;
        float glow = exp(-dist * 0.8) * 0.15;
        float rim  = exp(-dist * 6.0) * (1.2 + burst * 5.0);
        float arcs = smoothstep(0.03, 0.0, arc) * exp(-dist * 1.5) * 1.5;
        color += (kGlowColor * glow + kRimColor * rim + kArcColor * arcs) * flicker * min(dt, t1 - t);
        t += dt;
    }

    return vec4(color * (0.4 + 0.6 * energy), opacity);
}

// This is analytic so the build up costs nothing next to the marched effects around it
vec4 RiftImplosion(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest <= 0.0 || closest >= sceneT + radius)
    {
        return vec4(0.0);
    }

    float h       = length(toCenter - ray * closest);
    float pull    = progress * progress;
    float shell   = radius * 1.4 * (1.0 - pull);
    float flicker = 0.75 + 0.25 * Noise(vec3(u_time * 20.0, seed, 5.0));
    float angle   = atan(toCenter.y - ray.y * closest, length((toCenter - ray * closest).xz)) * 3.0;
    float streaks = smoothstep(0.6, 1.0, Noise(vec3(angle * 3.0, h * 0.5 + u_time * 12.0, seed)));
    vec3 color    = kRimColor * exp(-abs(h - shell) * 2.0) * (0.6 + pull * 2.5);
    color += kGlowColor * streaks * exp(-h / max(shell, 0.5)) * pull * 2.0;
    color += kRimColor * exp(-h * (4.0 - pull * 3.0)) * pull * 6.0;
    vec3 perp   = toCenter - ray * closest;
    vec3 side   = normalize(cross(ray, vec3(0.0, 1.0, 0.0)));
    float swirl = atan(dot(perp, cross(side, ray)), dot(perp, side));
    float arms  = smoothstep(0.75, 1.0, sin(swirl * 5.0 + log(h + 0.2) * 7.0 - u_time * 9.0 - pull * 25.0));
    color += mix(kGlowColor, kArcColor, pull) * arms * exp(-h / max(shell * 0.8, 0.5)) * pull * 3.5;
    color += vec3(1.0, 0.9, 0.95) * smoothstep(0.85, 1.0, progress) * exp(-h * 0.35) * 6.0;
    float core = smoothstep(0.35 + pull * 0.4, 0.0, h);
    return vec4(color * flicker, core * pull);
}

float Detonation(vec3 p, float radius, float seed, float progress)
{
    float grow   = 1.0 - pow(1.0 - min(progress / 0.07, 1.0), 3.0);
    float fade   = 1.0 - 0.5 * smoothstep(0.25, 0.9, progress);
    float len    = length(p);
    vec3 dir     = p / max(len, 0.0001);
    float spikes = 0.0;
    for (int k = 0; k < kDetonationSpikes; k++)
    {
        vec3 axis   = normalize(vec3(Hash(ivec3(k, int(seed), 1)), Hash(ivec3(k, int(seed), 2)), Hash(ivec3(k, int(seed), 3))) - 0.5);
        float twist = progress * 3.0 * (Hash(ivec3(k, int(seed), 5)) - 0.5);
        axis.xz     = mat2(cos(twist), -sin(twist), sin(twist), cos(twist)) * axis.xz;
        spikes      = max(spikes, pow(max(dot(dir, axis), 0.0), 60.0) * (0.6 + Hash(ivec3(k, int(seed), 4))));
    }

    float wobble = (Noise(dir * 5.0 + vec3(seed, u_time * 2.0, 0.0)) - 0.5) * 0.4;
    float star   = (len - radius * (0.7 + wobble * 0.4 + spikes * 3.5) * grow * fade) * 0.5;

    float pillarWidth = radius * 0.22 * grow * fade * (1.0 + (Noise(vec3(p.y * 0.12, seed, u_time * 3.0)) - 0.5) * 0.8);
    float pillar      = max(length(p.xz) - pillarWidth, abs(p.y) - radius * 9.5 * grow);

    float roll = radius * 4.0 * sqrt(progress);
    vec2 q     = vec2(length(p.xz) - roll, p.y * 2.5);
    float ring = (length(q) - radius * 0.3 * (1.0 - progress) * (0.6 + Noise(vec3(atan(p.z, p.x) * 6.0, seed, u_time)))) * 0.4;

    float tilt     = 1.1 + seed * 0.37;
    vec3 t         = vec3(p.x, p.y * cos(tilt) - p.z * sin(tilt), p.y * sin(tilt) + p.z * cos(tilt));
    vec2 q2        = vec2(length(t.xz) - radius * 3.2 * sqrt(progress), t.y * 3.0);
    float ring2    = (length(q2) - radius * 0.2 * (1.0 - progress)) * 0.4;
    float shape    = min(min(star, pillar), min(ring, ring2));
    float dissolve = smoothstep(0.45, 0.92, progress);
    return max(shape, (dissolve * 1.1 - Noise(p * 0.18 + vec3(seed, u_time * 0.5, 0.0))) * radius * 0.5);
}

vec4 RiftDetonation(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed, float jitter)
{
    float energy = 1.0 - progress;
    float burst  = exp(-progress * 10.0);
    vec3 fire    = vec3(1.0, 0.45, 0.15);
    vec3 color   = vec3(0.0);

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT)
    {
        float h = length(toCenter - ray * closest);
        for (int r = 0; r < 6; r++)
        {
            float ringR = radius * (2.5 + float(r) * 2.2) * pow(progress, 0.3 + float(r) * 0.1);
            vec3 tint   = mix(kRimColor, fire, float(r) * 0.18);
            color += tint * exp(-abs(h - ringR) * (1.2 - float(r) * 0.15)) * 2.2 * energy * energy;
        }

        float dome = radius * 14.0 * pow(progress, 0.35);
        color += mix(vec3(1.0, 0.9, 0.95), kRimColor, progress) * smoothstep(dome, dome * 0.97, h) * exp(-(dome - h) / radius) * 3.0 * energy;
    }

    vec3 beamPerp = vec3(-ray.z, 0.0, ray.x);
    float beamGap = abs(dot(toCenter, beamPerp)) / max(length(beamPerp), 0.001);
    float beamT   = dot(toCenter.xz, ray.xz) / max(dot(ray.xz, ray.xz), 0.001);
    if (beamT > 0.0 && beamT < sceneT && abs(ro.y + ray.y * beamT - center.y) < radius * 12.0)
    {
        float beamWidth = radius * (0.3 + 0.5 * burst) * (0.7 + 0.6 * Noise(vec3(u_time * 6.0, seed, 3.0)));
        color += mix(vec3(1.0, 0.9, 0.95), kRimColor, progress) * smoothstep(beamWidth, 0.0, beamGap) * 2.0 * energy * energy;
    }

    vec3 bend = toCenter - ray * closest;
    ray       = normalize(ray + bend * radius * 0.2 * energy / (dot(bend, bend) + radius));

    float bound = radius * 10.0;
    vec3 o      = ro - center;
    float b     = dot(o, ray);
    float disc  = b * b - dot(o, o) + bound * bound;
    if (disc <= 0.0)
    {
        return vec4(color, 0.0);
    }

    float root    = sqrt(disc);
    float t0      = max(-b - root, 0.0);
    float t1      = min(-b + root, sceneT);
    float opacity = 0.0;
    float flicker = 0.65 + 0.35 * Noise(vec3(u_time * 14.0, seed, 9.0));
    float t       = t0 + jitter * 0.2;
    for (int s = 0; s < kDetonationSteps && t < t1; s++)
    {
        vec3 p     = ro + ray * t - center;
        float dist = Detonation(p, radius, seed, progress);
        if (dist < 0.0)
        {
            color += Void(p / max(radius * 0.5, 0.5), ray, seed, u_time) * (1.0 + burst * 4.0);
            opacity = 1.0;
            break;
        }

        float dt     = clamp(dist * 0.8, 0.08, 6.0);
        float arc    = dist < 2.0 ? abs(Noise(p * 0.9 + vec3(seed, u_time * 4.0, 0.0)) - 0.5) : 1.0;
        float heat   = smoothstep(0.05, 0.5, progress);
        vec3 rimTint = mix(kRimColor, fire, heat * 0.7);
        float glow   = exp(-dist * 0.5) * 0.12;
        float rim    = exp(-dist * 4.0) * (1.5 + burst * 6.0);
        float arcs   = smoothstep(0.03, 0.0, arc) * exp(-dist * 1.0) * 1.8;
        color += (mix(kGlowColor, fire * 0.6, heat) * glow + rimTint * rim + kArcColor * arcs) * flicker * min(dt, t1 - t);
        t += dt;
    }

    color *= 0.65 * energy + 0.35 * (1.0 - smoothstep(0.85, 1.0, progress));
    float collapse = smoothstep(0.75, 0.97, progress);
    float pinch    = smoothstep(0.93, 0.97, progress) * (1.0 - smoothstep(0.97, 1.0, progress));
    if (closest > 0.0 && closest < sceneT + radius)
    {
        float h      = length(toCenter - ray * closest);
        float spiral = atan(dot(toCenter - ray * closest, vec3(0.0, 1.0, 0.0)), length((toCenter - ray * closest).xz)) * 6.0 + h * 0.3 - u_time * 6.0;
        color += kRimColor * exp(-abs(h - radius * 5.0 * (1.0 - collapse) * (1.0 - collapse)) * 1.2) * sin(collapse * 3.14159265) *
                 (1.5 + smoothstep(0.5, 1.0, sin(spiral)));
        color += vec3(1.0, 0.9, 0.95) * exp(-h * 0.6) * pinch * 8.0;
        color += kRimColor * exp(-abs(h - radius * 6.0 * smoothstep(0.97, 1.0, progress)) * 2.0) * pinch * 2.0;
    }

    return vec4(color, opacity);
}

vec3 Rainbow(float hue)
{
    return 0.5 + 0.5 * cos(6.2831853 * (hue + vec3(0.0, 0.33, 0.67)));
}

// This returns the distance between the ray and the segment, with t set to the ray distance at the closest point
float RaySegment(vec3 ro, vec3 ray, vec3 a, vec3 b, out float t)
{
    vec3 ba    = b - a;
    vec3 oa    = ro - a;
    float bd   = dot(ba, ray);
    float rdoa = dot(ray, oa);
    float s    = clamp((dot(ba, oa) - bd * rdoa) / max(dot(ba, ba) - bd * bd, 0.00001), 0.0, 1.0);
    t          = s * bd - rdoa;
    return length(oa + ray * t - ba * s);
}

vec3 TesseractWire(vec3 ro, vec3 ray, float sceneT, vec3 center, float size, float angle, float hueShift)
{
    vec3 verts[16];
    for (int i = 0; i < 16; i++)
    {
        vec4 v   = vec4(float(i & 1), float((i >> 1) & 1), float((i >> 2) & 1), float((i >> 3) & 1)) * 2.0 - 1.0;
        v.xw     = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * v.xw;
        v.yw     = mat2(cos(angle * 0.7), -sin(angle * 0.7), sin(angle * 0.7), cos(angle * 0.7)) * v.yw;
        v.zw     = mat2(cos(angle * 1.3), -sin(angle * 1.3), sin(angle * 1.3), cos(angle * 1.3)) * v.zw;
        v.xz     = mat2(cos(angle * 0.4), -sin(angle * 0.4), sin(angle * 0.4), cos(angle * 0.4)) * v.xz;
        verts[i] = center + v.xyz * (2.0 / (3.0 - v.w)) * size;
    }

    float width = max(size * 0.03, 0.04);
    vec3 color  = vec3(0.0);
    for (int i = 0; i < 16; i++)
    {
        float t;
        float d = RaySegment(ro, ray, verts[i], verts[i], t);
        if (t > 0.0 && t < sceneT)
        {
            color += vec3(1.0) * exp(-d / (width * 2.5)) * 2.0;
        }

        for (int bit = 0; bit < 4; bit++)
        {
            int j = i ^ (1 << bit);
            if (j < i)
            {
                continue;
            }

            d = RaySegment(ro, ray, verts[i], verts[j], t);
            if (t > 0.0 && t < sceneT)
            {
                vec3 tint = Rainbow(float(i) / 16.0 + float(bit) * 0.11 + hueShift);
                color += tint * (exp(-d / width) * 1.6 + exp(-d / (width * 8.0)) * 0.25);
            }
        }
    }

    return color;
}

vec3 RainbowRings(vec3 ro, vec3 ray, float sceneT, vec3 center, float ringRadius, float angle, float seed)
{
    vec3 color = vec3(0.0);
    for (int k = 0; k < 3; k++)
    {
        float tilt  = angle * (0.5 + float(k) * 0.3) + float(k) * 2.1 + seed;
        vec3 normal = normalize(vec3(sin(tilt), cos(tilt * 0.7 + float(k)), cos(tilt)));
        float t     = dot(center - ro, normal) / dot(ray, normal);
        if (t <= 0.0 || t >= sceneT)
        {
            continue;
        }

        vec3 local  = ro + ray * t - center;
        float r     = length(local);
        float ringR = ringRadius * (1.0 + float(k) * 0.35);
        float hue   = atan(dot(local, cross(normal, vec3(0.0, 1.0, 0.0))), local.y) / 6.2831853 + u_time * 0.5;
        color += Rainbow(hue + float(k) * 0.3) * exp(-abs(r - ringR) * (6.0 / max(ringRadius * 0.15, 0.5))) * 1.4;
    }

    return color;
}

vec3 RainbowColumn(vec3 ro, vec3 ray, float sceneT, vec3 center, float height, float width, float twist)
{
    float t;
    float d = RaySegment(ro, ray, center - vec3(0.0, height * 0.3, 0.0), center + vec3(0.0, height, 0.0), t);
    if (t <= 0.0 || t >= sceneT)
    {
        return vec3(0.0);
    }

    float y      = ro.y + ray.y * t - center.y;
    float strand = abs(sin(y * 0.6 - u_time * twist + atan(ray.z, ray.x)));
    float fade   = 1.0 - smoothstep(height * 0.6, height, y);
    return Rainbow(y * 0.06 - u_time * 0.8) * (exp(-d / width) * 1.5 + exp(-d / (width * 4.0)) * strand * 0.6) * fade;
}

vec4 ExperimentTesseract(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float appear  = smoothstep(0.0, 0.06, progress);
    float buildup = smoothstep(0.72, 0.97, progress);
    float ending  = smoothstep(0.97, 1.0, progress);
    float angle   = u_time * 1.2 + progress * progress * 30.0 + buildup * buildup * 40.0;
    float pulse   = 1.0 + 0.15 * sin(u_time * 9.0) * (1.0 - buildup);
    float size    = radius * 0.32 * appear * pulse * (1.0 + buildup * 0.5 - buildup * buildup * 0.4 + ending * 5.0);
    vec3 middle   = center + vec3(0.0, 5.0 * appear * (1.0 - buildup), 0.0);

    vec3 color = TesseractWire(ro, ray, sceneT, middle, size, angle, u_time * 0.4);
    color += TesseractWire(ro, ray, sceneT, middle, size * 0.45, -angle * 1.7, u_time * 0.4 + 0.5) * 0.8;
    color += RainbowRings(ro, ray, sceneT, middle, size * (1.8 - buildup), angle, seed) * (0.8 + buildup * 2.0);
    color += RainbowColumn(ro, ray, sceneT, middle, 40.0, 0.25 + buildup * 0.6, 6.0 + buildup * 20.0) * (0.4 + buildup * 1.6);

    vec3 toCenter = middle - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT)
    {
        vec3 offset  = toCenter - ray * closest;
        float h      = length(offset);
        float spiral = atan(offset.y, length(offset.xz)) * 4.0 + h * 0.8 - u_time * 5.0;
        color += Rainbow(h * 0.15 - u_time * 0.7 + sin(spiral) * 0.2) * exp(-h / max(size * 0.4, 0.3)) * (0.6 + buildup * 4.0);
    }

    return vec4(color * appear * (1.0 - ending), 0.0);
}

vec4 ExperimentBurst(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float energy = 1.0 - progress;
    vec3 color =
            TesseractWire(ro, ray, sceneT, center, radius * (0.3 + 1.5 * sqrt(progress)), u_time * 3.0 + seed, seed * 0.13) * energy * energy * 1.5;
    color += RainbowColumn(ro, ray, sceneT, center, 25.0 * sqrt(progress) + 1.0, 0.2 + radius * 0.05, 12.0) * energy * energy * 1.5;

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT + radius * 3.0)
    {
        float h     = length(toCenter - ray * closest);
        float shell = radius * 3.0 * sqrt(progress);
        color += Rainbow(h * 0.12 - progress * 3.0 + seed * 0.1) * exp(-abs(h - shell) * 1.2) * energy * energy * 3.0;
        color += Rainbow(h * 0.2 + 0.5 + seed * 0.1) * exp(-abs(h - shell * 0.6) * 2.0) * energy * energy * 1.5;
        color += Rainbow(seed * 0.1 + u_time) * exp(-h / (radius * 0.4)) * exp(-progress * 10.0) * 5.0;
    }

    return vec4(color, 0.0);
}

vec4 ExperimentFinale(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float energy = 1.0 - progress;
    float grow   = 1.0 - pow(1.0 - min(progress / 0.08, 1.0), 3.0);
    float spin   = u_time * 0.8 + progress * 8.0;
    vec3 color   = TesseractWire(ro, ray, sceneT, center, radius * (0.5 + 5.0 * sqrt(progress)), spin, -u_time * 0.4) * energy * energy * 1.5;
    color += TesseractWire(ro, ray, sceneT, center, radius * (0.3 + 2.5 * sqrt(progress)), -spin * 1.6, u_time * 0.5) * energy * energy;
    color += RainbowColumn(ro, ray, sceneT, center, 120.0 * grow, radius * 0.3 * energy + 0.1, 25.0) * energy * 2.0;

    for (int k = 0; k < 24; k++)
    {
        vec3 dir = normalize(vec3(Hash(ivec3(k, int(seed), 11)), Hash(ivec3(k, int(seed), 12)) * 0.8 + 0.1, Hash(ivec3(k, int(seed), 13))) -
                             vec3(0.5, 0.0, 0.5));
        float t;
        float d = RaySegment(ro, ray, center, center + dir * radius * 14.0 * grow, t);
        if (t > 0.0 && t < sceneT)
        {
            float along = length(ro + ray * t - center) / (radius * 14.0);
            color += Rainbow(along * 2.0 - u_time * 1.5 + float(k) * 0.04) * exp(-d / (radius * 0.1 * (0.3 + energy))) * 2.5 * energy * energy;
        }
    }

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT + radius * 10.0)
    {
        float h = length(toCenter - ray * closest);
        for (int r = 0; r < 8; r++)
        {
            float ringR = radius * (2.0 + float(r) * 2.0) * pow(progress, 0.3 + float(r) * 0.08);
            color += Rainbow(float(r) * 0.125 - u_time * 0.3) * exp(-abs(h - ringR) * (1.4 - float(r) * 0.1)) * 2.5 * energy * energy;
        }

        color += Rainbow(h * 0.05 - u_time) * exp(-h / (radius * 0.6)) * exp(-progress * 6.0) * 6.0;
        float last = smoothstep(0.85, 1.0, progress);
        color += Rainbow(h * 0.02 + u_time) * exp(-abs(h - radius * 16.0 * last) * 1.5) * sin(last * 3.14159265) * 2.0;
    }

    return vec4(color, 0.0);
}

float Fbm(vec3 p)
{
    return Noise(p) * 0.5 + Noise(p * 2.03 + 17.0) * 0.3 + Noise(p * 4.07 + 31.0) * 0.2;
}

vec3 Heat(float x)
{
    return vec3(1.6 * x, 1.3 * x * x, 1.4 * x * x * x * x);
}

// This returns how close the ray passes to a jagged bolt between a and b, with the ends pinned in place
float Bolt(vec3 ro, vec3 ray, float sceneT, vec3 a, vec3 b, int seed, float jag)
{
    float best = 1e9;
    vec3 prev  = a;
    for (int k = 1; k <= 8; k++)
    {
        float s   = float(k) / 8.0;
        vec3 kink = vec3(Hash(ivec3(seed, k, 1)), Hash(ivec3(seed, k, 2)), Hash(ivec3(seed, k, 3))) - 0.5;
        vec3 next = mix(a, b, s) + kink * jag * sin(s * 3.14159265);
        float t;
        float d = RaySegment(ro, ray, prev, next, t);
        if (t > 0.0 && t < sceneT)
        {
            best = min(best, d);
        }

        prev = next;
    }

    return best;
}

float ScreenAngle(vec3 ray, vec3 perp)
{
    vec3 side = normalize(cross(ray, vec3(0.0, 1.0, 0.0)));
    return atan(dot(perp, cross(side, ray)), dot(perp, side));
}

vec3 AccretionDisk(vec3 ro, vec3 ray, float limit, vec3 center, vec3 normal, float inner, float spin)
{
    float t = dot(center - ro, normal) / dot(ray, normal);
    if (t <= 0.0 || t >= limit)
    {
        return vec3(0.0);
    }

    vec3 local   = ro + ray * t - center;
    float r      = length(local);
    vec3 side    = normalize(cross(normal, vec3(0.0, 0.0, 1.0)));
    float angle  = atan(dot(local, cross(normal, side)), dot(local, side)) - spin * pow(inner / max(r, 0.01), 1.5);
    float lanes  = Fbm(vec3(cos(angle) * 3.0, sin(angle) * 3.0, log(max(r, 0.01)) * 9.0));
    float shape  = smoothstep(inner * 0.9, inner * 1.3, r) * smoothstep(inner * 6.0, inner * 2.0, r);
    float beam   = 1.0 + 0.9 * dot(cross(normal, local / max(r, 0.01)), -ray);
    float bright = pow(lanes, 2.5) * 4.0 * shape * beam * beam;
    return Heat(0.55 + 0.6 * inner / max(r, 0.01)) * bright;
}

vec4 SingularityCharge(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float form    = smoothstep(0.0, 0.25, progress);
    float crush   = smoothstep(0.86, 0.95, progress);
    float fade    = 1.0 - smoothstep(0.955, 0.975, progress);
    float horizon = radius * 0.3 * form * (1.0 + 0.6 * smoothstep(0.3, 0.8, progress)) * (1.0 - crush) + 0.001;
    float spin    = u_time * 2.0 + progress * progress * 40.0;
    vec3 normal   = normalize(vec3(sin(seed) * 0.3, 1.0, cos(seed) * 0.3));

    vec3 o      = ro - center;
    float b     = dot(o, ray);
    float disc  = b * b - dot(o, o) + horizon * horizon;
    float holeT = disc > 0.0 ? -b - sqrt(disc) : 1e9;
    bool hole   = holeT > 0.0 && holeT < sceneT;

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    vec3 perp     = toCenter - ray * closest;
    float h       = length(perp);

    // Bending the ray toward the hole lets the far side of the disk show up as an arch over it
    vec3 bent  = normalize(ray + perp * min(horizon * 2.0 / max(h * h, 0.0001), 2.0 / max(h, 0.0001)));
    vec3 color = AccretionDisk(ro, bent, hole ? holeT : sceneT, center, normal, horizon * 1.6, spin) * form;
    if (closest > 0.0 && closest < sceneT + radius)
    {
        float angle = ScreenAngle(ray, perp);
        color += vec3(1.0, 0.85, 0.6) * exp(-abs(h - horizon * 1.25) / (horizon * 0.04 + 0.02)) * form * 2.5;
        color += vec3(0.35, 0.25, 1.0) * exp(-abs(h - horizon * 2.8) / (horizon * 0.3 + 0.1)) * form * 0.6;
        float arms = smoothstep(0.7, 1.0, sin(angle * 3.0 + log(h + 0.2) * 9.0 + spin * 2.0));
        color += Heat(0.7) * arms * exp(-h / (radius * 1.5)) * smoothstep(horizon, horizon * 2.0, h) * form * 1.2;
        color += vec3(1.0, 0.95, 0.9) * exp(-h * 0.5) * crush * (1.0 - crush) * 24.0;
    }

    float jets = smoothstep(0.45, 0.75, progress) * (1.0 - crush);
    float jetT;
    float jetD = RaySegment(ro, ray, center - normal * radius * 9.0 * jets, center + normal * radius * 9.0 * jets, jetT);
    if (jetT > 0.0 && jetT < (hole ? holeT : sceneT))
    {
        float along = length(ro + ray * jetT - center);
        float knots = 0.6 + 0.4 * sin(along * 2.0 - u_time * 30.0);
        color += vec3(0.55, 0.75, 1.0) * (exp(-jetD / (0.08 + along * 0.02)) * 3.0 + exp(-jetD / (0.6 + along * 0.1)) * 0.5) * knots * jets;
    }

    return vec4(color * fade, hole ? fade : 0.0);
}

vec4 SingularityBlast(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float energy = 1.0 - progress;
    float grow   = 1.0 - pow(1.0 - min(progress / 0.1, 1.0), 3.0);
    vec3 normal  = normalize(vec3(sin(seed) * 0.3, 1.0, cos(seed) * 0.3));
    vec3 color   = vec3(0.0);
    for (int side = -1; side <= 1; side += 2)
    {
        float len = radius * 30.0 * grow;
        float t;
        float d = RaySegment(ro, ray, center, center + normal * float(side) * len, t);
        if (t > 0.0 && t < sceneT)
        {
            float along = length(ro + ray * t - center) / len;
            float width = radius * (0.12 + along * 0.5) * (0.4 + energy);
            float knots = 0.55 + 0.45 * sin(along * 50.0 - u_time * 35.0);
            color += mix(vec3(0.7, 0.85, 1.0), vec3(0.75, 0.35, 1.0), along) * (exp(-d / width) * 3.0 + exp(-d / (width * 5.0)) * 0.5) * knots *
                     energy * energy;
        }
    }

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT + radius * 4.0)
    {
        float h     = length(toCenter - ray * closest);
        float shell = radius * 4.0 * sqrt(progress);
        color += vec3(1.0, 0.97, 0.92) * smoothstep(shell, shell * 0.75, h) * exp(-progress * 7.0) * 5.0;
        color += Heat(0.6 + 0.5 * energy) * exp(-abs(h - shell) * 1.5) * 3.0 * energy;
        float front = radius * 16.0 * sqrt(progress);
        float wave  = pow(max(sin(h * 0.9 - progress * 70.0), 0.0), 10.0) * smoothstep(front, front * 0.9, h);
        color += vec3(0.5, 0.3, 1.0) * wave * exp(-h / (radius * 10.0)) * energy * 2.0;
    }

    float t = dot(center - ro, normal) / dot(ray, normal);
    if (t > 0.0 && t < sceneT)
    {
        vec3 local  = ro + ray * t - center;
        float r     = length(local);
        vec3 side   = normalize(cross(normal, vec3(0.0, 0.0, 1.0)));
        float angle = atan(dot(local, cross(normal, side)), dot(local, side)) + u_time * 0.8;
        float ringR = radius * 8.0 * pow(progress, 0.4);
        float clump = pow(Fbm(vec3(cos(angle) * 5.0, sin(angle) * 5.0, r * 0.25 - progress * 8.0)), 2.0) * 3.0;
        color += Heat(0.5 + 0.6 * energy) * exp(-abs(r - ringR) / (radius * (0.3 + progress * 1.5))) * clump * energy * 2.5;
    }

    return vec4(color, 0.0);
}

vec4 NovaCharge(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float form     = smoothstep(0.0, 0.2, progress);
    float swell    = smoothstep(0.15, 0.75, progress);
    float unstable = smoothstep(0.55, 0.85, progress);
    float crush    = smoothstep(0.85, 0.95, progress);
    float fade     = 1.0 - smoothstep(0.955, 0.975, progress);
    float throb    = 1.0 + 0.1 * sin(u_time * (3.0 + unstable * 35.0)) * (0.3 + unstable);
    float star     = radius * (0.25 + 0.55 * swell) * form * throb * (1.0 - crush * 0.97) + 0.001;
    float heat     = mix(0.9, 0.5, swell) + crush * 0.8;
    vec3 color     = vec3(0.0);
    float alpha    = 0.0;

    vec3 o      = ro - center;
    float b     = dot(o, ray);
    float disc  = b * b - dot(o, o) + star * star;
    float starT = 1e9;
    if (disc > 0.0 && -b - sqrt(disc) > 0.0 && -b - sqrt(disc) < sceneT)
    {
        starT        = -b - sqrt(disc);
        vec3 n       = (o + ray * starT) / star;
        float cells  = Fbm(n * (5.0 + unstable * 6.0) + vec3(seed, u_time * (0.4 + unstable * 2.0), 0.0));
        float spots  = smoothstep(0.6, 0.7, Fbm(n * 2.5 + vec3(0.0, u_time * 0.05, seed)));
        float limb   = pow(max(dot(n, -ray), 0.0), 0.45);
        float strobe = 1.0 + unstable * 0.5 * sin(u_time * 40.0 + cells * 20.0);
        color        = Heat(heat * (0.7 + 0.45 * cells) * (1.0 - 0.45 * spots)) * limb * 1.8 * strobe;
        alpha        = 1.0;
    }

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT + star * 4.0)
    {
        vec3 perp   = toCenter - ray * closest;
        float h     = length(perp);
        float angle = ScreenAngle(ray, perp);
        float above = max(h - star, 0.0);
        float rays  = pow(Noise(vec3(cos(angle) * 9.0, sin(angle) * 9.0, u_time * (1.5 + unstable * 6.0) - above * 0.3)), 3.0);
        color += Heat(heat) * (exp(-above / (star * 0.2 + 0.1)) * 1.5 + rays * exp(-above / (star * 1.5 + 0.1)) * 3.0) * form;
        color += vec3(0.8, 0.9, 1.0) * exp(-h * 0.4) * crush * (1.0 - crush) * 20.0;
    }

    for (int k = 0; k < 6; k++)
    {
        vec3 footA  = normalize(vec3(Hash(ivec3(k, int(seed), 21)), Hash(ivec3(k, int(seed), 22)), Hash(ivec3(k, int(seed), 23))) - 0.5);
        vec3 footB  = normalize(footA + normalize(cross(footA, vec3(0.0, 1.0, 0.0))) * (0.5 + Hash(ivec3(k, int(seed), 24)) * 0.6));
        float lift  = (0.4 + 0.8 * Hash(ivec3(k, int(seed), 25))) * (0.7 + 0.3 * sin(u_time * 1.7 + float(k))) * (1.0 + unstable * 1.5);
        vec3 prev   = center + footA * star;
        float width = star * 0.04 + 0.03;
        for (int j = 1; j <= 6; j++)
        {
            float s   = float(j) / 6.0;
            vec3 next = center + normalize(mix(footA, footB, s)) * star * (1.0 + lift * sin(s * 3.14159265));
            float t;
            float d = RaySegment(ro, ray, prev, next, t);
            if (t > 0.0 && t < min(starT, sceneT))
            {
                color += Heat(heat + 0.15) * (exp(-d / width) * 2.5 + exp(-d / (width * 6.0)) * 0.4) * form * (1.0 - crush);
            }

            prev = next;
        }
    }

    return vec4(color * fade, alpha * fade);
}

vec4 NovaBlast(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed, float jitter)
{
    float energy = 1.0 - progress;
    float burst  = exp(-progress * 12.0);
    vec3 color   = vec3(0.0);

    float shell = radius * 7.0 * pow(progress, 0.45);
    float thick = shell * 0.35 + 0.5;
    vec3 o      = ro - center;
    float b     = dot(o, ray);
    float disc  = b * b - dot(o, o) + (shell + thick) * (shell + thick);
    if (disc > 0.0)
    {
        float t0 = max(-b - sqrt(disc), 0.0);
        float t1 = min(-b + sqrt(disc), sceneT);
        float dt = (t1 - t0) / 14.0;
        for (int s = 0; s < 14 && t0 < t1; s++)
        {
            vec3 p      = o + ray * (t0 + (float(s) + jitter) * dt);
            float r     = length(p);
            vec3 dir    = p / max(r, 0.0001);
            float layer = exp(-abs(r - shell) / (thick * 0.5));
            float wisps = pow(1.0 - abs(Fbm(dir * 5.0 + vec3(seed, progress * 2.0, 0.0)) * 2.0 - 1.0), 6.0);
            vec3 tint   = mix(vec3(0.2, 0.9, 1.0), vec3(1.0, 0.25, 0.15), Noise(dir * 3.0 + seed));
            color += tint * wisps * layer * dt / thick * 3.0 * energy;
        }
    }

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0 && closest < sceneT + radius * 2.0)
    {
        float h = length(toCenter - ray * closest);
        color += vec3(1.0, 0.95, 0.85) * exp(-h / (radius * (0.5 + progress * 4.0))) * burst * 10.0;
        color += Heat(0.9) * exp(-abs(h - radius * 13.0 * pow(progress, 0.3)) * 0.8) * energy * energy * 2.0;
        color += vec3(0.8, 0.9, 1.0) * exp(-h / 0.25) * smoothstep(0.03, 0.1, progress) * 4.0;
    }

    float spin   = u_time * 12.0;
    vec3 axis    = vec3(sin(0.6) * cos(spin), cos(0.6), sin(0.6) * sin(spin));
    float pulsar = smoothstep(0.04, 0.15, progress) * energy;
    float t;
    float d = RaySegment(ro, ray, center - axis * radius * 40.0, center + axis * radius * 40.0, t);
    if (t > 0.0 && t < sceneT)
    {
        color += vec3(0.6, 0.85, 1.0) * (exp(-d / (radius * 0.08)) * 3.0 + exp(-d / (radius * 0.5)) * 0.5) * pulsar;
    }

    return vec4(color, 0.0);
}

vec4 TempestCharge(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float form   = smoothstep(0.0, 0.2, progress);
    float fury   = smoothstep(0.25, 0.92, progress);
    float fade   = 1.0 - smoothstep(0.955, 0.975, progress);
    float height = 18.0;
    vec3 top     = center + vec3(0.0, height, 0.0);
    vec3 color   = vec3(0.0);
    float alpha  = 0.0;

    float cloudT = (top.y - ro.y) / ray.y;
    if (cloudT > 0.0 && cloudT < sceneT)
    {
        vec3 local  = ro + ray * cloudT - top;
        float r     = length(local.xz);
        float swirl = atan(local.z, local.x) + u_time * (0.6 + fury * 2.5) * radius * 1.5 / (r + radius * 0.5);
        float dens  = Fbm(vec3(cos(swirl) * r * 0.15, sin(swirl) * r * 0.15, u_time * 0.2 + seed));
        dens = smoothstep(0.35, 0.7, dens) * smoothstep(radius * 4.0 * form, radius * 2.4 * form, r) * smoothstep(radius * 0.3, radius * 0.8, r);
        float strike = step(1.0 - (0.05 + fury * 0.3), Hash(ivec3(int(u_time * 12.0), int(seed), 4)));
        float cell   = step(0.92, Hash(ivec3(int(floor(local.x / 6.0)), int(floor(local.z / 6.0)), int(u_time * 10.0))));
        color += (vec3(0.07, 0.08, 0.13) + vec3(0.5, 0.6, 1.0) * (strike * 0.9 + cell * 0.7)) * dens;
        color += vec3(0.4, 0.6, 1.0) * exp(-r / (radius * 0.4)) * form * (0.5 + fury * 2.0);
        alpha = dens * 0.85;
    }

    float t;
    float d = RaySegment(ro, ray, center, top, t);
    if (t > 0.0 && t < sceneT)
    {
        float y      = ro.y + ray.y * t - center.y;
        float width  = radius * (0.15 + 0.5 * pow(y / height, 2.0)) * form;
        float strand = 0.5 + 0.5 * sin(y * 1.5 - u_time * 20.0 + atan(ray.z, ray.x) * 3.0);
        color += vec3(0.45, 0.55, 0.9) * (exp(-d / max(width, 0.01)) * 0.6 + smoothstep(width, width * 0.5, d) * strand * 0.5) * (0.3 + fury);
    }

    float rate = 5.0 + fury * 8.0;
    for (int k = 0; k < 8; k++)
    {
        if (float(k) >= 2.0 + fury * 6.0)
        {
            break;
        }

        float clock = u_time * rate + Hash(ivec3(k, int(seed), 9)) * 10.0;
        int bolt    = int(floor(clock)) * 31 + k * 7 + int(seed);
        float vis   = (1.0 - smoothstep(0.0, 0.4, fract(clock))) * step(0.35, Hash(ivec3(bolt, 1, 1)));
        vec3 from   = top + vec3(Hash(ivec3(bolt, 2, 1)) - 0.5, 0.0, Hash(ivec3(bolt, 3, 1)) - 0.5) * radius * 5.0 * form;
        vec3 to     = center + vec3(Hash(ivec3(bolt, 4, 1)) - 0.5, 0.0, Hash(ivec3(bolt, 5, 1)) - 0.5) * radius * 3.0 - vec3(0.0, 0.5, 0.0);
        float gap   = Bolt(ro, ray, sceneT, from, to, bolt, height * 0.35);
        color += (vec3(0.85, 0.9, 1.0) * exp(-gap / 0.08) * 4.0 + vec3(0.4, 0.3, 1.0) * exp(-gap / 1.2) * 0.6) * vis * form;
    }

    return vec4(color * fade, alpha * fade);
}

vec4 TempestBlast(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float energy = 1.0 - progress;
    float burst  = exp(-progress * 5.0);
    float strobe = 0.4 + 0.6 * step(0.45, Hash(ivec3(int(u_time * 20.0), int(seed), 2)));
    int frame    = int(u_time * 18.0);
    vec3 color   = vec3(0.0);
    for (int k = 0; k < 3; k++)
    {
        vec3 sky  = center + vec3(Hash(ivec3(k, frame, 5)) - 0.5, 0.0, Hash(ivec3(k, frame, 6)) - 0.5) * 20.0 + vec3(0.0, 120.0, 0.0);
        float gap = Bolt(ro, ray, sceneT, sky, center, frame * 3 + k + int(seed) * 13, 24.0);
        color += (vec3(0.9, 0.93, 1.0) * exp(-gap / (0.25 + burst * 0.6)) * 6.0 + vec3(0.5, 0.35, 1.0) * exp(-gap / 4.0) * 1.2) * burst * strobe;
    }

    for (int k = 0; k < 12; k++)
    {
        float angle = float(k) / 12.0 * 6.2831853 + Hash(ivec3(k, int(seed), 7)) * 0.5;
        float len   = radius * 9.0 * sqrt(progress) + 1.0;
        vec3 end    = center + vec3(cos(angle) * len, -0.3, sin(angle) * len);
        float gap   = Bolt(ro, ray, sceneT, center, end, k * 17 + int(u_time * 24.0) * 131 + int(seed), len * 0.25);
        color += (vec3(0.9, 0.95, 1.0) * exp(-gap / 0.1) * 3.0 + vec3(0.3, 0.5, 1.0) * exp(-gap / 1.0) * 0.5) * energy * energy * strobe;
    }

    float t = (center.y - ro.y) / ray.y;
    if (t > 0.0 && t < sceneT + 1.0)
    {
        vec3 local  = ro + ray * t - center;
        float r     = length(local.xz);
        float angle = atan(local.z, local.x);
        float arc   = abs(Noise(vec3(cos(angle) * 10.0, sin(angle) * 10.0, u_time * 8.0)) - 0.5);
        float ringR = radius * 9.0 * sqrt(progress);
        color += vec3(0.4, 0.6, 1.0) * exp(-abs(r - ringR) * 1.5) * (1.0 + smoothstep(0.05, 0.0, arc) * 3.0) * energy * energy * 2.0;
    }

    for (int k = 0; k < 10; k++)
    {
        float roll  = Hash(ivec3(k, int(seed), 8));
        float angle = roll * 6.2831853 + u_time * (1.0 + roll);
        float orbit = radius * (1.0 + progress * 6.0 * roll);
        vec3 orb    = center + vec3(cos(angle) * orbit, progress * radius * 4.0 * (0.5 + roll) + sin(u_time * 3.0 + float(k)), sin(angle) * orbit);
        float along = dot(orb - ro, ray);
        if (along > 0.0 && along < sceneT)
        {
            float gap = length(orb - ro - ray * along);
            color += mix(vec3(0.4, 0.9, 1.0), vec3(0.9, 0.4, 1.0), roll) * (exp(-gap / 0.3) * 4.0 + exp(-gap / 1.5) * 0.6) * energy *
                     (0.7 + 0.3 * sin(u_time * 40.0 + float(k)));
        }
    }

    return vec4(color, 0.0);
}

float Crystal(vec3 p, float fold, float angle, out float trap)
{
    mat2 twist = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
    float size = 1.0;
    trap       = 1e9;
    for (int i = 0; i < 6; i++)
    {
        p = abs(p);
        if (p.x < p.y)
        {
            p.xy = p.yx;
        }

        if (p.x < p.z)
        {
            p.xz = p.zx;
        }

        if (p.y < p.z)
        {
            p.yz = p.zy;
        }

        p.xy = twist * p.xy;
        p    = p * fold - vec3(1.0) * (fold - 1.0);
        size *= fold;
        trap = min(trap, dot(p, p));
    }

    vec3 q = abs(p) - vec3(1.0);
    return (length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0)) / size;
}

vec4 PrismCharge(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed, float jitter)
{
    float form  = smoothstep(0.0, 0.25, progress);
    float wild  = smoothstep(0.6, 0.93, progress);
    float fade  = 1.0 - smoothstep(0.955, 0.975, progress);
    float size  = radius * 0.45 * form * (1.0 + 0.3 * wild) + 0.001;
    float angle = u_time * 0.3 + wild * wild * sin(u_time * 7.0) * 0.8;
    float fold  = 2.0 + 0.25 * sin(u_time * 0.7 + seed) + wild * 0.6 * sin(u_time * 5.3);
    float turn  = u_time * 0.5;
    mat2 spin   = mat2(cos(turn), -sin(turn), sin(turn), cos(turn));
    vec3 middle = center + vec3(0.0, 4.0 * form, 0.0);
    vec3 color  = vec3(0.0);
    float alpha = 0.0;
    float bound = size * 1.8;
    vec3 o      = ro - middle;
    float b     = dot(o, ray);
    float disc  = b * b - dot(o, o) + bound * bound;
    float trap  = 0.0;
    if (disc > 0.0)
    {
        float t  = max(-b - sqrt(disc), 0.0) + jitter * size * 0.02;
        float t1 = min(-b + sqrt(disc), sceneT);
        for (int s = 0; s < 48 && t < t1; s++)
        {
            vec3 q  = (o + ray * t) / size;
            q.xz    = spin * q.xz;
            float d = Crystal(q, fold, angle, trap) * size;
            color += Rainbow(trap * 0.15 + u_time * 0.4) * exp(-d / (size * 0.03)) * 0.04 * (1.0 + wild * 2.0);
            if (d < size * 0.002)
            {
                vec2 e     = vec2(0.002, -0.002);
                vec3 n     = normalize(e.xyy * Crystal(q + e.xyy, fold, angle, trap) + e.yyx * Crystal(q + e.yyx, fold, angle, trap) +
                                       e.yxy * Crystal(q + e.yxy, fold, angle, trap) + e.xxx * Crystal(q + e.xxx, fold, angle, trap));
                vec3 r     = ray;
                r.xz       = spin * r.xz;
                float face = abs(dot(n, r));
                color += Rainbow(face * 1.5 + trap * 0.3 + u_time * 0.3) * (0.4 + pow(1.0 - face, 3.0) * 2.5);
                color += vec3(1.0) * pow(max(reflect(r, n).y, 0.0), 24.0) * 2.0;
                alpha = 0.8;
                break;
            }

            t += d * 0.9;
        }
    }

    vec3 inward = normalize(vec3(cos(u_time * 0.4 + seed), 0.35, sin(u_time * 0.4 + seed)));
    float t;
    float d = RaySegment(ro, ray, middle + inward * radius * 20.0 * form, middle, t);
    if (t > 0.0 && t < sceneT)
    {
        color += vec3(1.0) * (exp(-d / 0.12) * 3.0 + exp(-d / 0.8) * 0.3) * form;
    }

    float heading = atan(-inward.z, -inward.x);
    for (int k = 0; k < 7; k++)
    {
        float spread = heading + (float(k) - 3.0) * 0.07 * (1.0 + wild * 3.0 * sin(u_time * 3.0));
        vec3 outward = normalize(vec3(cos(spread), -0.35 + (float(k) - 3.0) * 0.02, sin(spread)));
        d            = RaySegment(ro, ray, middle, middle + outward * radius * 22.0 * form, t);
        if (t > 0.0 && t < sceneT)
        {
            color += Rainbow(float(k) / 7.0) * (exp(-d / 0.15) * 2.5 + exp(-d / 1.0) * 0.3) * form;
        }
    }

    return vec4(color * fade, alpha * fade);
}

vec4 PrismBlast(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float energy = 1.0 - progress;
    float grow   = 1.0 - pow(1.0 - min(progress / 0.1, 1.0), 3.0);
    vec3 color   = vec3(0.0);

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0)
    {
        vec3 perp     = toCenter - ray * closest;
        float h       = length(perp);
        float segment = 6.2831853 / 12.0;
        float folded  = abs(mod(ScreenAngle(ray, perp) + u_time * 0.3, segment) - segment * 0.5);
        vec2 mirror   = vec2(cos(folded), sin(folded)) * h;
        float lattice = abs(sin(mirror.x * 0.8 - progress * 30.0) * sin(mirror.y * 1.6 + u_time * 2.0));
        float bloom   = smoothstep(0.0, 0.05, progress) * (1.0 - smoothstep(0.3, 0.8, progress));
        color += Rainbow(h * 0.03 + folded * 2.0 - u_time * 0.5) * smoothstep(0.8, 1.0, lattice) * exp(-h / (radius * 14.0 * grow + 0.1)) * bloom *
                 1.5;
        for (int k = 0; k < 7; k++)
        {
            float ringR = radius * (6.0 + float(k) * 0.8) * pow(progress, 0.4);
            color += Rainbow(float(k) / 7.0) * exp(-abs(h - ringR) * 2.0) * energy * energy * 1.5;
        }

        color += vec3(1.0) * exp(-h / (radius * 0.5)) * exp(-progress * 10.0) * 8.0;
    }

    for (int k = 0; k < 40; k++)
    {
        vec3 dir    = normalize(vec3(Hash(ivec3(k, int(seed), 31)), Hash(ivec3(k, int(seed), 32)), Hash(ivec3(k, int(seed), 33))) - 0.5);
        float roll  = Hash(ivec3(k, int(seed), 34));
        float reach = radius * (6.0 + roll * 10.0) * sqrt(progress);
        vec3 shard  = center + dir * reach - vec3(0.0, progress * progress * radius * 4.0, 0.0);
        float turn  = u_time * (4.0 + roll * 8.0);
        vec3 axis   = normalize(vec3(cos(turn), sin(turn * 0.7), sin(turn)));
        float len   = radius * 0.25 * (0.5 + roll);
        float t;
        float d = RaySegment(ro, ray, shard - axis * len, shard + axis * len, t);
        if (t > 0.0 && t < sceneT)
        {
            color += Rainbow(roll + u_time * 0.5 + progress) * (exp(-d / 0.06) * 3.0 + exp(-d / 0.6) * 0.4) * energy *
                     (0.5 + 0.5 * sin(u_time * 20.0 + float(k)));
        }

        d = RaySegment(ro, ray, center + dir * reach * 0.5, shard, t);
        if (t > 0.0 && t < sceneT)
        {
            color += Rainbow(roll + progress * 2.0) * exp(-d / 0.08) * 0.6 * energy * energy;
        }
    }

    return vec4(color, 0.0);
}

float Heartbeat(float beats)
{
    float phase = fract(beats);
    return exp(-phase * 25.0) + exp(-max(phase - 0.2, 0.0) * 25.0) * step(0.2, phase) * 0.7;
}

// The eye is a billboard, so it stares straight at whoever is looking at it
vec4 HellEye(vec3 ro, vec3 ray, float sceneT, vec3 center, float size, float open, float dilate, float seed)
{
    vec3 normal = normalize(ro - center);
    float t     = dot(center - ro, normal) / dot(ray, normal);
    if (t <= 0.0 || t >= sceneT || open <= 0.0)
    {
        return vec4(0.0);
    }

    vec3 side  = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
    vec3 q     = ro + ray * t - center;
    vec2 uv    = vec2(dot(q, side), dot(q, cross(normal, side))) / size;
    float lid  = open * 0.5 * (1.0 - uv.x * uv.x);
    float edge = abs(uv.y) - lid;
    vec3 color = vec3(1.0, 0.04, 0.02) * exp(-max(edge, 0.0) * 10.0) * smoothstep(1.3, 0.9, abs(uv.x)) * open * 1.5;
    if (edge >= 0.0 || abs(uv.x) >= 1.0)
    {
        return vec4(color, 0.0);
    }

    int glance       = int(floor(u_time * 1.3));
    vec2 look        = vec2(Hash(ivec3(glance, int(seed), 40)) - 0.5, (Hash(ivec3(glance, int(seed), 41)) - 0.5) * 0.4) * 0.35 * (1.0 - dilate);
    vec2 iris        = uv - look;
    float r          = length(iris);
    float veins      = pow(1.0 - abs(Noise(vec3(uv * 7.0, seed)) * 2.0 - 1.0), 10.0);
    vec3 sclera      = vec3(0.12, 0.01, 0.0) + vec3(0.8, 0.02, 0.0) * veins * smoothstep(0.25, 0.9, r);
    float striations = 0.5 + 0.5 * Noise(vec3(atan(iris.y, iris.x) * 5.0, r * 14.0, u_time * 0.5));
    vec3 irisColor   = Heat(0.55 + 0.4 * striations) * vec3(1.0, 0.3, 0.15) * (1.5 + dilate * 3.0);
    float pupil      = length(vec2(iris.x / (0.04 + 0.22 * dilate), iris.y / (0.33 - 0.05 * dilate)));
    color            = mix(sclera, irisColor, smoothstep(0.4, 0.37, r)) * smoothstep(0.85, 1.05, pupil);
    color *= 0.3 + 0.7 * smoothstep(0.0, 0.08, -edge);
    return vec4(color, 1.0);
}

vec4 HellSpike(vec3 ro, vec3 ray, float sceneT, vec3 base, vec3 tip, float width)
{
    float t;
    float d = RaySegment(ro, ray, base, tip, t);
    if (t <= 0.0 || t >= sceneT)
    {
        return vec4(0.0);
    }

    float along = clamp(dot(ro + ray * t - base, tip - base) / max(dot(tip - base, tip - base), 0.0001), 0.0, 1.0);
    float w     = width * (1.0 - along) + 0.01;
    vec3 rim    = vec3(1.0, 0.1, 0.02) * exp(-abs(d - w) / 0.05) * (1.5 + along * 2.0);
    return d < w ? vec4(vec3(0.03, 0.0, 0.0) + rim * 0.5, 1.0) : vec4(rim * 0.6 + vec3(0.6, 0.0, 0.0) * exp(-(d - w) / 0.6) * 0.3, 0.0);
}

vec3 BloodRain(vec3 ro, vec3 ray, float sceneT, vec3 center, float area, float amount, float seed)
{
    vec3 color = vec3(0.0);
    for (int k = 0; k < 48; k++)
    {
        if (float(k) >= amount * 48.0)
        {
            break;
        }

        vec3 column = center + vec3(Hash(ivec3(k, int(seed), 50)) - 0.5, 0.0, Hash(ivec3(k, int(seed), 51)) - 0.5) * area;
        float fall  = fract(u_time * (0.5 + Hash(ivec3(k, int(seed), 52)) * 0.4) + Hash(ivec3(k, int(seed), 53)));
        vec3 drop   = column + vec3(0.0, 30.0 * (1.0 - fall), 0.0);
        float t;
        float d = RaySegment(ro, ray, drop, drop + vec3(0.0, 2.5, 0.0), t);
        if (t > 0.0 && t < sceneT)
        {
            color += vec3(0.7, 0.0, 0.0) * (exp(-d / 0.05) * 1.5 + exp(-d / 0.4) * 0.2);
        }
    }

    return color;
}

vec4 HellgateCharge(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float seconds = progress * 15.2;
    float beat    = Heartbeat((50.0 * seconds + 50.0 * seconds * seconds * seconds / (15.2 * 15.2)) / 60.0) * smoothstep(0.0, 0.1, progress);
    float carve   = smoothstep(0.02, 0.3, progress);
    float awaken  = smoothstep(0.35, 0.7, progress);
    float frenzy  = smoothstep(0.7, 0.93, progress);
    float black   = smoothstep(0.93, 0.96, progress) * smoothstep(radius * 14.0, radius * 6.0, length(ro - center));
    float fade    = 1.0 - smoothstep(0.975, 0.99, progress);
    vec3 color    = vec3(0.0);
    float alpha   = 0.0;

    if (ray.y > 0.0)
    {
        vec3 swirl   = ray * 6.0 + vec3(0.0, -u_time * 0.3, 0.0);
        float clouds = Fbm(swirl + vec3(atan(ray.z, ray.x) * 2.0 + u_time * 0.2 * (1.0 + frenzy * 4.0), 0.0, seed));
        float stain  = awaken * smoothstep(radius * 20.0, radius * 8.0, length(ro - center)) * smoothstep(0.0, 0.25, ray.y);
        color += (vec3(0.25, 0.0, 0.0) * clouds + vec3(0.8, 0.05, 0.0) * pow(clouds, 4.0) * (1.0 + beat)) * stain;
        alpha = max(alpha, stain * 0.55);
    }

    float floorT = (center.y - 0.49 - ro.y) / ray.y;
    if (floorT > 0.0 && floorT < sceneT + 0.4)
    {
        vec2 local   = (ro + ray * floorT - center).xz;
        float r      = length(local);
        float angle  = atan(local.y, local.x);
        float outer  = radius * 3.0;
        float inner  = outer * 0.82;
        float reveal = step(angle / 6.2831853 + 0.5, carve);
        float lines  = exp(-abs(r - outer) / 0.08) + exp(-abs(r - inner) / 0.06) + exp(-abs(r - outer * 0.3) / 0.05);

        int rune    = int(floor((angle + u_time * 0.05) / 6.2831853 * 36.0));
        vec2 cell   = vec2(fract((angle + u_time * 0.05) / 6.2831853 * 36.0), (r - inner) / (outer - inner)) * 3.0;
        float glyph = r > inner + 0.15 && r < outer - 0.15 ? step(0.55, Hash(ivec3(rune, int(cell.x) * 3 + int(cell.y), int(seed)))) : 0.0;
        glyph *= smoothstep(0.5, 0.3, abs(fract(cell.x) - 0.5)) * smoothstep(0.5, 0.3, abs(fract(cell.y) - 0.5));

        float star = 1e9;
        for (int k = 0; k < 5; k++)
        {
            float a0 = float(k) * 1.2566371 + u_time * 0.05;
            float a1 = float(k + 2) * 1.2566371 + u_time * 0.05;
            vec2 p0  = vec2(cos(a0), sin(a0)) * inner;
            vec2 ba  = vec2(cos(a1), sin(a1)) * inner - p0;
            star     = min(star, length(local - p0 - ba * clamp(dot(local - p0, ba) / dot(ba, ba), 0.0, 1.0)));
        }

        float sigil = (lines + glyph * 0.8 + exp(-star / 0.07)) * reveal;
        float pool  = smoothstep(outer * awaken, outer * awaken - 1.0, r) * awaken;
        float waves = 0.5 + 0.5 * sin(r * 3.0 - u_time * 4.0 - beat * 3.0);
        color += vec3(1.0, 0.03, 0.01) * sigil * (0.8 + beat * 2.5 + frenzy * 2.0) * (1.0 - pool * 0.4);
        color += vec3(0.35, 0.0, 0.0) * pool * (0.4 + 0.6 * Fbm(vec3(local * 0.5, u_time * 0.3)) + waves * 0.3) * (0.6 + beat);
        alpha = max(alpha, pool * 0.8);
    }

    for (int k = 0; k < 9; k++)
    {
        float angle = float(k) / 9.0 * 6.2831853;
        float rise  = smoothstep(0.45 + float(k) * 0.02, 0.6 + float(k) * 0.02, progress);
        vec3 base   = center + vec3(cos(angle) * radius * 3.4, -0.5, sin(angle) * radius * 3.4);
        vec4 spike  = HellSpike(ro, ray, sceneT, base,
                                base + vec3(-cos(angle) * 2.0, 9.0 + Hash(ivec3(k, int(seed), 60)) * 5.0, -sin(angle) * 2.0) * rise, 0.9 * rise);
        color       = color * (1.0 - spike.a) + spike.rgb * (1.0 + beat);
        alpha       = max(alpha, spike.a);
    }

    vec3 eye = center + vec3(0.0, 28.0, 0.0);
    color += BloodRain(ro, ray, sceneT, center, radius * 8.0, smoothstep(0.25, 0.8, progress), seed);
    for (int k = -1; k <= 1; k += 2)
    {
        float t;
        vec3 from = eye + vec3(float(k) * 1.5, -2.0 * awaken, 0.0);
        float d   = RaySegment(ro, ray, from, center + vec3(float(k) * 2.0, -0.5, 0.0), t);
        if (t > 0.0 && t < sceneT)
        {
            float y = (ro.y + ray.y * t - center.y) / 28.0;
            color += vec3(0.8, 0.0, 0.0) * exp(-d / (0.08 + 0.1 * sin(y * 30.0 - u_time * 6.0) * 0.5)) * 1.5 *
                     smoothstep(1.0 - (awaken - 0.3) * 1.5, 1.0, y);
        }
    }

    float band    = floor(v_uv.y * 90.0);
    float corrupt = step(1.0 - (0.02 + frenzy * 0.2) * smoothstep(radius * 16.0, radius * 6.0, length(ro - center)),
                         Hash(ivec3(int(band), int(u_time * 20.0), 61)));
    color         = mix(color, vec3(0.5, 0.0, 0.0) * Hash(ivec3(int(band), int(u_time * 20.0), 62)), corrupt);
    alpha         = max(alpha, corrupt * 0.6);
    color *= 1.0 - black;
    alpha = max(alpha, black * 0.97);

    vec4 stare = HellEye(ro, ray, sceneT, eye, 7.0 * (1.0 + 0.06 * beat),
                         awaken * (1.0 - smoothstep(0.88, 0.9, progress) * (1.0 - smoothstep(0.9, 0.92, progress))), frenzy, seed);
    color      = color * (1.0 - stare.a) + stare.rgb * (1.0 + black * 2.0);
    alpha      = max(alpha, stare.a);
    return vec4(color * fade, alpha * fade);
}

vec4 HellgateBlast(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    float energy = 1.0 - progress;
    float open   = smoothstep(0.0, 0.06, progress) * (1.0 - smoothstep(0.7, 0.95, progress));
    float height = 60.0;
    vec3 color   = vec3(0.0);
    float alpha  = 0.0;

    if (ray.y > -0.2)
    {
        float clouds = Fbm(ray * 5.0 + vec3(u_time * 0.4, 0.0, seed));
        float stain  = energy * smoothstep(radius * 30.0, radius * 10.0, length(ro - center));
        color += (vec3(0.4, 0.0, 0.0) * clouds + vec3(1.0, 0.15, 0.0) * pow(clouds, 5.0) * 2.0) * stain;
        alpha = stain * 0.6;
    }

    float floorT = (center.y - 0.49 - ro.y) / ray.y;
    if (floorT > 0.0 && floorT < sceneT + 0.4)
    {
        vec2 local  = (ro + ray * floorT - center).xz;
        float r     = length(local);
        float angle = atan(local.y, local.x);
        float ringR = radius * 12.0 * sqrt(progress);
        float crack = pow(1.0 - abs(Noise(vec3(cos(angle) * 6.0, sin(angle) * 6.0, r * 0.15)) * 2.0 - 1.0), 18.0);
        crack += pow(1.0 - abs(Noise(vec3(local * 0.3, seed)) * 2.0 - 1.0), 30.0);
        color += Heat(0.5 + 0.4 * energy) * vec3(1.0, 0.4, 0.2) * crack * smoothstep(ringR, ringR * 0.8, r) * (0.7 + 0.3 * sin(u_time * 3.0 + r)) *
                 energy * 3.0;
        color += vec3(1.0, 0.05, 0.0) * exp(-abs(r - ringR) / (0.4 + progress * 2.0)) * energy * energy * 4.0;
    }

    for (int k = 0; k < 16; k++)
    {
        float angle = (float(k) + Hash(ivec3(k, int(seed), 70)) * 0.6) / 16.0 * 6.2831853;
        float reach = radius * (2.0 + Hash(ivec3(k, int(seed), 71)) * 4.0);
        float rise =
                smoothstep(reach / (radius * 12.0) * reach / (radius * 12.0), reach / (radius * 12.0) * reach / (radius * 12.0) + 0.03, progress);
        rise *= 1.0 - smoothstep(0.85, 1.0, progress);
        vec3 base  = center + vec3(cos(angle) * reach, -0.5 - (1.0 - rise) * 3.0, sin(angle) * reach);
        vec4 spike = HellSpike(ro, ray, sceneT, base,
                               base + vec3(cos(angle) * 5.0, 14.0 + Hash(ivec3(k, int(seed), 72)) * 10.0, sin(angle) * 5.0) * rise, 1.6 * rise);
        color      = color * (1.0 - spike.a) + spike.rgb;
        alpha      = max(alpha, spike.a);
    }

    float t;
    float d = RaySegment(ro, ray, center, center + vec3(0.0, 120.0 * smoothstep(0.0, 0.1, progress), 0.0), t);
    if (t > 0.0 && t < sceneT)
    {
        float y     = ro.y + ray.y * t - center.y;
        float fire  = Fbm(vec3(atan(ray.z, ray.x) * 3.0, y * 0.15 - u_time * 4.0, seed));
        float width = radius * (0.8 + fire * 1.2) * (0.4 + energy);
        color += Heat(0.5 + fire * 0.6) * vec3(1.0, 0.35, 0.2) * (exp(-d / width) * 2.5 + exp(-d / (width * 4.0)) * 0.5) * energy;
    }

    vec3 gate   = center + vec3(0.0, height * 0.5, 0.0);
    vec3 facing = normalize(vec3(ro.x - gate.x, 0.0, ro.z - gate.z));
    t           = dot(gate - ro, facing) / dot(ray, facing);
    if (t > 0.0 && t < sceneT)
    {
        vec3 q    = ro + ray * t - gate;
        float u   = dot(q, vec3(-facing.z, 0.0, facing.x));
        float v   = q.y / (height * 0.5);
        float w   = open * radius * 1.4 * max(1.0 - v * v, 0.0) * (0.7 + 0.6 * Noise(vec3(v * 8.0, u_time * 0.5, seed)));
        float gap = abs(u) - w;
        color += vec3(1.0, 0.5, 0.2) * exp(-max(gap, 0.0) / 0.25) * 3.0 * open + vec3(1.0, 0.05, 0.0) * exp(-max(gap, 0.0) / 3.0) * open;
        if (gap < 0.0)
        {
            float depth = Fbm(vec3(u * 0.3, v * 3.0 - u_time * 1.5, u_time * 0.3 + seed));
            vec3 inside = vec3(0.05, 0.0, 0.0) + Heat(depth * 0.9) * vec3(1.0, 0.25, 0.1) * pow(depth, 2.0) * 3.0;
            vec4 stare  = HellEye(ro, ray, sceneT, gate, open * radius * 1.3, smoothstep(0.12, 0.25, progress) * open,
                                  0.2 + 0.6 * sin(u_time * 2.0) * 0.5, seed);
            color       = mix(inside * (1.0 + smoothstep(0.0, -0.4, gap)), stare.rgb, stare.a);
            alpha       = 1.0;
        }
    }

    for (int k = 0; k < 40; k++)
    {
        float roll  = Hash(ivec3(k, int(seed), 80));
        float climb = fract(u_time * (0.08 + roll * 0.1) + Hash(ivec3(k, int(seed), 81)));
        vec3 ember  = center + vec3((Hash(ivec3(k, int(seed), 82)) - 0.5) * radius * 10.0 + sin(u_time * 2.0 + float(k)) * 1.5, climb * 40.0,
                                    (roll - 0.5) * radius * 10.0 + cos(u_time * 1.7 + float(k)) * 1.5);
        float along = dot(ember - ro, ray);
        if (along > 0.0 && along < sceneT)
        {
            float gap = length(ember - ro - ray * along);
            color += Heat(0.8 - climb * 0.4) * (exp(-gap / 0.07) * 3.0 + exp(-gap / 0.5) * 0.3) * (1.0 - climb) * energy;
        }
    }

    vec3 toCenter = center - ro;
    float closest = dot(toCenter, ray);
    if (closest > 0.0)
    {
        float h = length(toCenter - ray * closest);
        color += vec3(1.0, 0.85, 0.8) * exp(-h / (radius * (0.5 + progress * 3.0))) * exp(-progress * 14.0) * 12.0;
    }

    color += BloodRain(ro, ray, sceneT, center, radius * 12.0, energy, seed + 1.0);
    return vec4(color, alpha);
}

// This draws in screen space, so only a camera inside the radius sees it
vec4 PhaseGlitch(vec3 center, float radius, float progress, float seed)
{
    float strength = (1.0 - progress) * step(length(center), radius);
    if (strength <= 0.0)
    {
        return vec4(0.0);
    }

    int frame   = int(floor(u_time * 30.0)) + int(seed) * 131;
    int band    = int(floor(v_uv.y * mix(18.0, 50.0, Hash(ivec3(frame, 5, 0)))));
    float tear  = step(1.0 - strength * 0.45, Hash(ivec3(frame, band, 11)));
    float block = step(1.0 - strength * 0.12, Hash(ivec3(ivec2(floor(v_uv * vec2(28.0, 16.0))), frame)));
    float edge  = smoothstep(0.25, 0.75, length(v_uv - 0.5));
    vec3 tint   = mix(vec3(0.55, 0.25, 1.0), vec3(0.2, 0.9, 1.0), Hash(ivec3(frame, band, 13)));
    float grain = Hash(ivec3(ivec2(gl_FragCoord.xy), frame));
    vec3 color  = tint * (tear * (0.4 + 0.6 * grain) + block * 0.8 + edge * 0.6) * strength;
    return vec4(color, clamp(tear * 0.35 + block * 0.6, 0.0, 0.85) * strength);
}

vec4 FracturePulse(vec3 ro, vec3 ray, float sceneT, vec3 center, float radius, float progress, float seed)
{
    vec3 o      = ro - center;
    float b     = dot(o, ray);
    float outer = radius * 1.15;
    float disc  = outer * outer - (dot(o, o) - b * b);
    if (disc <= 0.0)
    {
        return vec4(0.0);
    }

    float tNear = max(-b - sqrt(disc), 0.0);
    float tFar  = min(-b + sqrt(disc), sceneT);
    if (tFar <= tNear)
    {
        return vec4(0.0);
    }

    float lead  = radius * sqrt(progress);
    float ghost = radius * sqrt(max(progress - 0.18, 0.0)) * 0.9;
    vec3 color  = vec3(0.0);
    for (int k = 0; k < 20; k++)
    {
        vec3 p       = o + ray * mix(tNear, tFar, (float(k) + 0.5) / 20.0);
        float r      = length(p);
        float wobble = (Noise(p / max(r, 0.01) * 5.0 + vec3(seed, u_time * 2.0, 0.0)) - 0.5) * 0.35;
        color += vec3(0.65, 0.35, 1.0) * exp(-abs(r - lead - wobble) * 6.0);
        color += vec3(0.25, 0.85, 1.0) * exp(-abs(r - ghost - wobble) * 5.0) * 0.6;
    }

    return vec4(color * (tFar - tNear) / 20.0 * (1.0 - progress) * (1.0 - progress) * 2.0, 0.0);
}

// The crack sits inside whatever was hit, so we let it through a bit of depth
vec4 FractureCrack(vec3 ro, vec3 ray, float sceneT, vec3 center, float size, float progress, float seed)
{
    float closest = dot(center - ro, ray);
    if (closest <= 0.0 || length(center - ro - ray * closest) > size * 1.4)
    {
        return vec4(0.0);
    }

    float grow  = smoothstep(0.0, 0.12, progress);
    float limit = sceneT + size * 0.6;
    float crack = 0.0;
    for (int k = 0; k < 9; k++)
    {
        vec3 dir    = normalize(vec3(Hash(ivec3(int(seed), k, 50)), Hash(ivec3(int(seed), k, 52)), Hash(ivec3(int(seed), k, 53))) - 0.5);
        float reach = size * (0.6 + 0.6 * Hash(ivec3(int(seed), k, 51))) * grow;
        vec3 prev   = center;
        for (int j = 1; j <= 4; j++)
        {
            vec3 kink =
                    vec3(Hash(ivec3(int(seed), k * 8 + j, 54)), Hash(ivec3(int(seed), k * 8 + j, 55)), Hash(ivec3(int(seed), k * 8 + j, 56))) - 0.5;
            vec3 next = center + (dir + kink * 0.5) * reach * float(j) / 4.0;
            float t;
            float d = RaySegment(ro, ray, prev, next, t);
            if (t > 0.0 && t < limit)
            {
                crack += exp(-d / (0.012 + t * 0.0012)) * (1.0 - float(j) * 0.15);
            }

            if (j == 2)
            {
                vec3 branch = next + normalize(kink + dir.yzx * 0.6) * reach * 0.35;
                d           = RaySegment(ro, ray, next, branch, t);
                if (t > 0.0 && t < limit)
                {
                    crack += exp(-d / (0.01 + t * 0.001)) * 0.6;
                }
            }

            prev = next;
        }
    }

    float fade  = 1.0 - smoothstep(0.5, 1.0, progress);
    float flash = exp(-length(center - ro - ray * closest) / size * 6.0) * pow(1.0 - progress, 4.0);
    crack       = min(crack, 3.0);
    return vec4((vec3(0.85, 0.95, 1.0) * crack * 1.6 + vec3(0.6, 0.3, 1.0) * flash * 2.0) * fade, clamp(crack * 0.3, 0.0, 0.5) * fade);
}

vec4 FractureTear(vec3 ro, vec3 ray, float sceneT, vec3 center, float size, float progress, float seed)
{
    vec3 normal = normalize(vec3(ro.x - center.x, 0.0, ro.z - center.z) + vec3(0.0001, 0.0, 0.0));
    float t     = dot(center - ro, normal) / dot(ray, normal);
    if (t <= 0.0 || t >= sceneT)
    {
        return vec4(0.0);
    }

    vec3 side  = normalize(cross(vec3(0.0, 1.0, 0.0), normal));
    vec3 q     = ro + ray * t - center;
    vec2 uv    = vec2(dot(q, side), q.y) / size;
    float open = smoothstep(0.0, 0.12, progress) * (1.0 - smoothstep(0.82, 1.0, progress));
    float jag  = (Noise(vec3(uv.y * 7.0, u_time * 1.5, seed)) - 0.5) * 0.16;
    float edge = abs(uv.x - jag) - open * 0.24 * pow(max(1.0 - uv.y * uv.y, 0.0), 0.7);
    float halo = exp(-length(uv) * 2.5) * open * 0.4;
    vec3 rim   = vec3(0.75, 0.45, 1.0) * exp(-abs(edge) * size * 12.0) * open * 2.5;
    if (edge >= 0.0 || abs(uv.y) >= 1.0)
    {
        return vec4(rim + vec3(0.5, 0.3, 1.0) * halo, 0.0);
    }

    return vec4(Void(vec3(uv, 0.0), ray, seed, u_time) + rim, 1.0);
}

void main()
{
    // Bobbing moves the eye off the origin, so we take the ray start from the near plane
    vec2 ndc     = v_uv * 2.0 - 1.0;
    vec4 nearPos = u_invViewProj * vec4(ndc, -1.0, 1.0);
    vec4 farPos  = u_invViewProj * vec4(ndc, 1.0, 1.0);
    vec4 hitPos  = u_invViewProj * vec4(ndc, texture(u_depthTexture, v_uv).r * 2.0 - 1.0, 1.0);
    vec4 midNear = u_invViewProj * vec4(0.0, 0.0, -1.0, 1.0);
    vec4 midFar  = u_invViewProj * vec4(0.0, 0.0, 1.0, 1.0);
    vec3 ro      = nearPos.xyz / nearPos.w;
    vec3 rd      = normalize(farPos.xyz / farPos.w - ro);
    float sceneT = length(hitPos.xyz / hitPos.w - ro);
    // Interleaved gradient noise spreads the step offsets evenly, and shifting it per frame lets the resolve average it out
    vec2 noiseCoord = gl_FragCoord.xy + 5.588238 * u_frame;
    float jitter    = fract(52.9829189 * fract(dot(noiseCoord, vec2(0.06711056, 0.00583715))));
    vec3 forward = normalize(midFar.xyz / midFar.w - midNear.xyz / midNear.w);
    vec3 right   = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    int frame    = int(floor(u_time * 24.0));

    vec3 color         = vec3(0.0);
    float transmission = 1.0;
    for (int i = 0; i < int(u_effectCount); i++)
    {
        int type       = int(u_effectStates[i].x);
        float progress = u_effectStates[i].y;
        float seed     = u_effectStates[i].z;
        bool hell      = type == kEffectHellgateCharge || type == kEffectHellgateBlast;
        float glitch   = type >= kEffectExperimentTesseract ? 0.2 * (1.0 - progress) : 1.0 - progress;
        glitch         = type == kEffectHellgateCharge ? 0.1 + 0.6 * progress * progress * progress : glitch;
        glitch         = type == kEffectHellgateBlast ? 0.8 * (1.0 - progress) : glitch;

        int glitchFrame = frame + int(seed) * 977;
        int band        = int(floor(v_uv.y * mix(24.0, 70.0, Hash(ivec3(glitchFrame, 3, 0)))));
        float bandTear  = step(1.0 - glitch * 0.5, Hash(ivec3(glitchFrame, band, 31)));
        vec3 ray        = normalize(rd + right * (Hash(ivec3(band, glitchFrame, 7)) - 0.5) * 0.12 * bandTear);

        vec4 effect = vec4(0.0);
        if (type == kEffectRiftShockwave)
        {
            effect = RiftShockwave(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed, jitter);
        }
        else if (type == kEffectRiftDetonation)
        {
            effect = RiftDetonation(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed, jitter);
        }
        else if (type == kEffectRiftImplosion)
        {
            effect = RiftImplosion(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectExperimentTesseract)
        {
            effect = ExperimentTesseract(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectExperimentBurst)
        {
            effect = ExperimentBurst(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectExperimentFinale)
        {
            effect = ExperimentFinale(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectSingularityCharge)
        {
            effect = SingularityCharge(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectSingularityBlast)
        {
            effect = SingularityBlast(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectNovaCharge)
        {
            effect = NovaCharge(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectNovaBlast)
        {
            effect = NovaBlast(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed, jitter);
        }
        else if (type == kEffectTempestCharge)
        {
            effect = TempestCharge(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectTempestBlast)
        {
            effect = TempestBlast(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectPrismCharge)
        {
            effect = PrismCharge(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed, jitter);
        }
        else if (type == kEffectPrismBlast)
        {
            effect = PrismBlast(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectHellgateCharge)
        {
            effect = HellgateCharge(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectHellgateBlast)
        {
            effect = HellgateBlast(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectPhaseGlitch)
        {
            effect = PhaseGlitch(u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectFracturePulse)
        {
            effect = FracturePulse(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectFractureCrack)
        {
            effect = FractureCrack(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }
        else if (type == kEffectFractureTear)
        {
            effect = FractureTear(ro, ray, sceneT, u_effects[i].xyz, u_effects[i].w, progress, seed);
        }

        // Void surfaces stay solid until the end, so we fade effects out before they are dropped
        bool charge = type == kEffectSingularityCharge || type == kEffectNovaCharge || type == kEffectTempestCharge || type == kEffectPrismCharge ||
                      type == kEffectHellgateCharge;
        effect.a *= type == kEffectRiftImplosion || charge ? 1.0 : 1.0 - smoothstep(0.7, 1.0, progress);
        effect.rgb *= type == kEffectRiftShockwave ? 1.0 - smoothstep(0.7, 1.0, progress) : 1.0;
        effect.rgb = mix(effect.rgb, hell ? effect.rrr * vec3(1.8, 0.1, 0.05) : effect.gbr * vec3(1.6, 0.4, 0.7), bandTear * glitch);
        effect.rgb += (Hash(ivec3(ivec2(gl_FragCoord.xy), glitchFrame)) - 0.5) * 0.3 * glitch * effect.a;
        color += transmission * max(effect.rgb, vec3(0.0));
        transmission *= 1.0 - effect.a;
    }

    color += u_flash.rgb * u_flash.a * transmission;
    FragColor = vec4(color, 1.0 - transmission * (1.0 - u_flash.a));
}
