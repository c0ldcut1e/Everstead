#version 450

layout(location = 0) in vec2 v_uv;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_view;
    vec4 u_origin;
    float u_aspect;
    float u_travel;
    float u_warp;
    float u_open;
    float u_close;
    float u_stability;
    float u_time;
    float u_twist;
    float u_blend;
    float u_fade;
};

layout(location = 0) out vec4 FragColor;

const float kFar        = 80.0;
const int kMaxSteps     = 72;
const int kVoidSteps    = 32;
const int kMaxBounces   = 5;
const float kRefraction = 1.05;

float Box(vec3 p, vec3 size)
{
    vec3 q = abs(p) - size;
    return min(max(q.x, max(q.y, q.z)), 0.0) + length(max(q, 0.0));
}

// We hash integer cells because float sin and tan hashes come out different on every driver
vec3 CellColor(ivec3 cell)
{
    uvec3 v = uvec3(cell.x, cell.y, cell.z & 255) * 1664525u + 1013904223u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    v ^= v >> 16u;
    v.x += v.y * v.z;
    v.y += v.z * v.x;
    v.z += v.x * v.y;
    vec3 color = vec3(v >> 8u) / 16777215.0 * vec3(1.0, 0.6, 1.0);
    return length(color) < 1.15 ? vec3(0.0) : normalize(color);
}

// The caller wraps u_travel by 256 cells, which is the period CellColor masks z to
float Walls(vec3 p, out vec3 color)
{
    float twist = p.z * 0.012 * u_twist;
    p.xy        = mat2(cos(twist), sin(twist), -sin(twist), cos(twist)) * p.xy;
    p.z += u_travel;
    color = CellColor(ivec3(floor(p / 2.0 + 0.5)));
    p.yz  = mod(p.yz - 1.0, 2.0) - 1.0;
    p.x   = abs(p.x) - 28.0;
    return Box(p, vec3(0.7)) - 0.05;
}

float Surface(vec3 p)
{
    vec3 color;
    return abs(Walls(p, color)) - 0.001;
}

float Trace(vec3 origin, vec3 direction, inout vec3 glow, out float steps, out vec3 hitColor)
{
    float t               = 0.0;
    float surfaceDistance = 1.0;
    steps                 = 1.0;
    for (int i = 1; i < kMaxSteps; i++)
    {
        steps      = float(i);
        float wall = Walls(origin + direction * t, hitColor);
        if (surfaceDistance > 0.01)
        {
            glow += exp(-wall * 0.1) * hitColor * 0.5;
        }

        surfaceDistance = abs(wall) - 0.001;
        // The twist stretches the distance field, so we take shorter steps to avoid marching through walls
        t += surfaceDistance * 0.75;
        if (surfaceDistance < 0.0001 || t > kFar)
        {
            break;
        }
    }

    return t;
}

vec3 Sky(vec3 direction, vec3 glow, float steps)
{
    return length(pow(abs(direction + vec3(0.0, 0.5, 0.0)), vec3(3.0))) * vec3(0.1, 0.0, 0.2) + glow / steps;
}

// This matches the void the tear shader draws inside the crack, so a transition started there has no seam
vec3 Void(vec3 direction)
{
    vec3 glow   = vec3(0.0);
    vec3 hitColor;
    float t     = 0.0;
    float steps = 1.0;
    for (int i = 1; i < kVoidSteps; i++)
    {
        steps      = float(i);
        float wall = abs(Walls(u_origin.xyz + direction * t, hitColor)) - 0.001;
        glow += exp(-wall * 0.1) * hitColor * 0.5;
        t += wall * 0.75;
        if (wall < 0.0001 || t > kFar)
        {
            break;
        }
    }

    vec3 sky   = Sky(direction, glow, steps);
    vec3 color = t > kFar ? sky : hitColor * 1.4 * exp(-t * 0.02) + sky;
    return color + vec3(0.75, 0.45, 1.0) * exp(-length(direction.xy) * 6.0) * 0.6;
}

float Hash(uint value)
{
    value = value * 747796405u + 2891336453u;
    value = ((value >> ((value >> 28u) + 4u)) ^ value) * 277803737u;
    value ^= value >> 22u;
    return float(value >> 8u) / 16777215.0;
}

void main()
{
    float instability = 1.0 - u_stability;
    uint glitchFrame  = uint(floor(u_time * 24.0));
    float burst       = step(0.55, Hash(uint(floor(u_time * 3.0)) * 13u + 5u)) * instability;
    float band        = floor(v_uv.y * mix(24.0, 70.0, Hash(glitchFrame * 3u)));
    float tear        = step(1.0 - instability * (0.25 + 0.45 * burst), Hash(glitchFrame * 31u + uint(band)));
    vec2 blockCell    = floor(v_uv * vec2(32.0, 18.0));
    uint blockHash    = uint(blockCell.x) * 73u + uint(blockCell.y) * 9277u + glitchFrame * 131u;
    float corrupt     = step(1.0 - instability * (0.06 + 0.25 * burst), Hash(blockHash));
    vec2 glitchUv     = v_uv + vec2((Hash(uint(band) + glitchFrame * 7u) - 0.5) * 0.35 * tear * instability, 0.0);
    glitchUv += (vec2(Hash(blockHash + 1u), Hash(blockHash + 2u)) - 0.5) * 0.4 * corrupt;
    glitchUv     = mix(glitchUv, floor(glitchUv * 48.0) / 48.0, corrupt);
    vec2 uv      = (glitchUv - 0.5) * vec2(u_aspect, 1.0);
    float radius = length(uv);
    // We nudge x because atan(0, 0) is undefined and returns different values per driver
    float angle     = atan(uv.y, uv.x + 0.00001);
    float maxRadius = length(vec2(u_aspect, 1.0)) * 0.5 + 0.1;
    float wobble    = sin(angle * 7.0 + u_travel * 3.14159265 / 64.0) * (0.04 + instability * 0.1) +
                      sin(angle * 23.0 - u_travel * 3.14159265 / 16.0) * instability * 0.05;
    float openEdge  = radius + wobble + 0.1 - smoothstep(0.0, 1.0, u_open) * (maxRadius + 0.1);
    float closeEdge = radius + wobble + 0.1 - smoothstep(0.0, 1.0, u_close) * (maxRadius + 0.1);
    // The wobble can push the close edge below zero at the center, so it only cuts in once closing starts
    float cover = (1.0 - smoothstep(-0.03, 0.03, openEdge)) * (u_close > 0.0 ? smoothstep(-0.03, 0.03, closeEdge) : 1.0);
    float rim   = exp(-abs(openEdge) * 30.0) * sin(u_open * 3.14159265) + exp(-abs(closeEdge) * 30.0) * sin(u_close * 3.14159265);
    if (cover + rim < 0.002)
    {
        FragColor = vec4(0.0);
        return;
    }

    // The view looks down negative z, so we turn it around to face down the tunnel
    vec3 view        = mat3(u_view) * vec3(uv, -u_origin.w * (1.0 - u_warp * 0.45));
    vec3 direction   = normalize(vec3(-view.x, view.y, -view.z));
    vec3 lookDir     = direction;
    vec3 voidColor   = u_blend < 1.0 ? Void(direction) : vec3(0.0);
    vec3 origin      = u_origin.xyz;
    vec3 light       = vec3(0.5, 0.5, 0.8);
    vec3 glow        = vec3(0.0);
    vec4 accumulated = vec4(0.0);
    float steps      = 1.0;
    int reflections  = 0;
    for (int i = 0; i < kMaxBounces && accumulated.a < 1.0 && u_blend > 0.0; i++)
    {
        vec3 hitColor;
        float t = Trace(origin, direction, glow, steps, hitColor);
        if (t > kFar)
        {
            break;
        }

        vec3 sky     = Sky(direction, glow, steps);
        vec3 hit     = origin + direction * t;
        float center = Surface(hit);
        vec3 normal  = normalize(
                center - vec3(Surface(hit - vec3(0.001, 0.0, 0.0)), Surface(hit - vec3(0.0, 0.001, 0.0)), Surface(hit - vec3(0.0, 0.0, 0.001))));
        origin         = hit - normal * 0.01;
        vec3 refracted = refract(direction, normal, i % 2 == 0 ? 1.0 / kRefraction : kRefraction);
        if (dot(refracted, refracted) == 0.0 && reflections <= 0)
        {
            refracted   = reflect(direction, normal);
            reflections = 2;
        }

        if (max(reflections, 0) % 3 == 0)
        {
            direction = refracted;
        }

        reflections--;

        float diffuse  = clamp(length(normal * light), 0.0, 1.0);
        vec3 fresnel   = pow(1.0 - diffuse, 3.0) * mix(sky, vec3(0.4), 0.5);
        float specular = (1.0 - length(cross(refracted, normal * light))) * 0.2;
        float ao       = min(Surface(hit + normal * 0.3) - 0.3, 0.3) * 0.5;
        vec3 color     = max(hitColor * (diffuse + 1.0) + fresnel * (hitColor + 1.0) + specular + ao + glow / steps, vec3(0.0));
        float opacity  = length(hitColor) < 0.5 ? 0.0 : 0.1;
        accumulated += vec4(color * opacity, opacity) * (1.0 - accumulated.a);
    }

    accumulated.rgb += Sky(direction, glow, steps) * max(1.0 - accumulated.a, 0.0);

    accumulated.rgb += vec3(0.75, 0.45, 1.0) * exp(-length(lookDir.xy) * 6.0) * step(0.0, lookDir.z) * (0.3 + u_warp * 1.2);

    float lane     = length(lookDir.xy) / max(abs(lookDir.z), 0.05);
    float laneHash = Hash(uint(floor((atan(lookDir.y, lookDir.x + 0.00001) / 6.2831853 + 0.5) * 96.0)));
    // Travel wraps at 512, so the streak phase uses a divisor that keeps whole cycles across the wrap
    float streak = pow(fract(laneHash * 7.0 + lane * 0.5 - u_travel / 64.0), 8.0);
    accumulated.rgb += vec3(0.6, 0.8, 1.0) * streak * step(0.85, laneHash) * smoothstep(0.2, 1.2, lane) * u_warp;
    accumulated.rgb *= 1.0 - 0.5 * radius * radius;
    accumulated.rgb = mix(voidColor, accumulated.rgb, u_blend);
    accumulated.rgb = mix(accumulated.rgb, accumulated.gbr * vec3(1.6, 0.4, 0.7), tear * instability);
    accumulated.rgb = mix(accumulated.rgb, floor((1.0 - accumulated.brg) * 4.0) / 4.0 * vec3(1.0, 0.2, 0.3), corrupt);
    float scanline  = Hash(uint(gl_FragCoord.y) + glitchFrame * 977u);
    accumulated.rgb += (Hash(uint(gl_FragCoord.x) * 7919u + uint(gl_FragCoord.y) * 104729u + glitchFrame) - 0.5) * 0.35 * instability;
    accumulated.rgb += vec3(1.0, 0.1, 0.15) * step(0.985, scanline) * instability;
    accumulated.rgb =
            mix(accumulated.rgb, accumulated.rgb * vec3(1.5, 0.35, 0.4) + vec3(0.12, 0.0, 0.0), instability * (0.35 + 0.35 * sin(u_time * 9.0)));
    accumulated.rgb *= 1.0 - instability * 0.7 * step(0.8, Hash(glitchFrame));

    accumulated.rgb += vec3(0.8, 0.5, 1.0) * rim * 2.0;
    float alpha = clamp(cover + rim, 0.0, 1.0) * u_fade;
    FragColor   = vec4(pow(clamp(accumulated.rgb, 0.0, 1.0), vec3(0.8)) * alpha, alpha);
}
