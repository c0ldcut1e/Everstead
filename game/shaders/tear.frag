#version 450
#extension GL_GOOGLE_include_directive : require

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_depthTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invViewProj;
    vec4 u_tears[16];
    vec4 u_tearStates[16];
    float u_tearCount;
    float u_time;
};

layout(location = 0) out vec4 FragColor;

const float kHeight   = 2.6;
const vec3 kBounds    = vec3(0.8, 1.6, 0.8);
const int kSteps      = 40;

#include "void.glsl"

// This is the signed distance to the twisted crack sheet, negative inside the void
float Crack(vec3 p, float seed, float open, out float taper)
{
    float grow = smoothstep(0.0, 0.6, open);
    float y    = p.y / (kHeight * grow) + 0.5;
    taper      = pow(sin(clamp(y, 0.0, 1.0) * 3.14159265), 0.7);

    vec2 center = vec2(sin(p.y * 2.3 + seed + u_time * 0.4), sin(p.y * 1.9 - seed * 1.3 + u_time * 0.33)) * 0.12;
    float angle = p.y * 1.1 + seed + sin(u_time * 0.3 + seed) * 0.4;
    vec2 q      = mat2(cos(angle), -sin(angle), sin(angle), cos(angle)) * (p.xz - center);

    float width     = 0.3 * taper * smoothstep(0.25, 1.0, open) * (0.7 + 0.6 * Noise(vec3(p.y * 6.0, seed * 3.0, u_time * 0.8)));
    float thickness = (0.025 + 0.03 * taper) * grow;
    vec2 e          = abs(q) - vec2(width, thickness);
    float d         = length(max(e, 0.0)) + min(max(e.x, e.y), 0.0);
    return d + (Noise(p * 5.0 + vec3(0.0, u_time * 0.6, seed)) - 0.5) * 0.08;
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
    float jitter = Hash(ivec3(ivec2(gl_FragCoord.xy), 7));
    vec3 forward = normalize(midFar.xyz / midFar.w - midNear.xyz / midNear.w);
    vec3 right   = normalize(cross(forward, vec3(0.0, 1.0, 0.0)));
    vec3 up      = cross(right, forward);
    int frame    = int(floor(u_time * 24.0));

    vec3 color         = vec3(0.0);
    float transmission = 1.0;
    for (int i = 0; i < int(u_tearCount); i++)
    {
        vec3 center = u_tears[i].xyz;
        float seed  = floor(u_tears[i].w);
        // We ease in so the tear bursts open on the peak of the open sound and snaps shut on the close one
        float open        = u_tearStates[i].x * u_tearStates[i].x;
        float flash       = pow(open, 6.0) * (1.0 - open) * 17.7;
        float instability = u_tearStates[i].y;
        float sway        = sin(u_time * 0.2 + seed) * 0.3 + (1.0 - open) * (u_tearStates[i].z > 0.5 ? 1.5 : -1.5);
        mat2 rotation     = mat2(cos(sway), -sin(sway), sin(sway), cos(sway));

        int glitchFrame = frame + int(seed) * 977;
        float burst     = step(0.55, Hash(ivec3(int(floor(u_time * 3.0)), int(seed), 5))) * instability;
        int band        = int(floor(v_uv.y * mix(24.0, 70.0, Hash(ivec3(glitchFrame, 3, 0)))));
        float bandTear  = step(1.0 - instability * (0.25 + 0.45 * burst), Hash(ivec3(glitchFrame, band, 31)));
        ivec2 block     = ivec2(floor(v_uv * vec2(32.0, 18.0)));
        float corrupt   = step(1.0 - instability * (0.06 + 0.25 * burst), Hash(ivec3(block, glitchFrame)));
        vec2 shift      = vec2((Hash(ivec3(band, glitchFrame, 7)) - 0.5) * 0.12 * bandTear, 0.0) +
                          (vec2(Hash(ivec3(block, glitchFrame + 1)), Hash(ivec3(block, glitchFrame + 2))) - 0.5) * 0.15 * corrupt;
        vec3 ray        = normalize(rd + right * shift.x + up * shift.y);

        vec3 o     = (ro - center) / kBounds;
        vec3 d     = ray / kBounds;
        float a    = dot(d, d);
        float b    = dot(o, d);
        float disc = b * b - a * (dot(o, o) - 1.0);
        if (disc <= 0.0)
        {
            continue;
        }

        float root = sqrt(disc);
        float t0   = max((-b - root) / a, 0.0);
        float t1   = min((-b + root) / a, sceneT);
        if (t1 <= t0)
        {
            continue;
        }

        vec3 tearColor    = vec3(0.0);
        float tearOpacity = 0.0;
        float flicker     = 0.75 + 0.25 * Noise(vec3(u_time * 9.0, seed, 0.0));
        float t           = t0 + jitter * 0.02;
        for (int s = 0; s < kSteps && t < t1; s++)
        {
            vec3 p = ro + ray * t - center;
            p.xz   = rotation * p.xz;
            float taper;
            float dist = Crack(p, seed, open, taper);
            if (dist < 0.0)
            {
                tearColor += Void(p, ray, seed, u_time);
                tearOpacity = 1.0;
                break;
            }

            // We step further the farther we are from the sheet, since the glow there is too faint to need fine samples
            float dt   = clamp(dist * 0.6, 0.015, 0.2);
            float arc  = dist < 0.3 ? abs(Noise(p * 7.0 + vec3(seed, u_time * 2.5, 0.0)) - 0.5) : 1.0;
            float glow = exp(-dist * 6.0) * 0.6 * taper;
            float rim  = exp(-dist * 45.0) * (3.0 + flash * 4.0) * taper;
            float arcs = smoothstep(0.035, 0.0, arc) * exp(-dist * 9.0) * 2.5 * taper;
            tearColor += (kGlowColor * glow + kRimColor * rim + kArcColor * arcs) * flicker * min(dt, t1 - t);
            t += dt;
        }

        tearColor = mix(tearColor, tearColor.gbr * vec3(1.6, 0.4, 0.7), bandTear * instability);
        tearColor = mix(tearColor, floor((1.0 - tearColor.brg) * 4.0) / 4.0 * vec3(1.0, 0.2, 0.3) * tearOpacity, corrupt);
        tearColor += (Hash(ivec3(ivec2(gl_FragCoord.xy), glitchFrame)) - 0.5) * 0.35 * instability * tearOpacity;
        tearColor += vec3(1.0, 0.1, 0.15) * step(0.985, Hash(ivec3(int(gl_FragCoord.y), glitchFrame, 11))) * instability * tearOpacity;
        tearColor = mix(tearColor, tearColor * vec3(1.5, 0.35, 0.4) + vec3(0.12, 0.0, 0.0) * tearOpacity,
                        instability * (0.35 + 0.35 * sin(u_time * 9.0)));
        tearColor *= 1.0 - instability * 0.7 * step(0.8, Hash(ivec3(glitchFrame, 0, 0)));
        color += transmission * max(tearColor, vec3(0.0));
        transmission *= 1.0 - tearOpacity;
        if (transmission <= 0.0)
        {
            break;
        }
    }

    FragColor = vec4(color, 1.0 - transmission);
}
