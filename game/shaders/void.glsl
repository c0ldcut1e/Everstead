const int kVoidSteps   = 32;
const float kVoidFar   = 80.0;
const vec3 kRimColor  = vec3(0.95, 0.85, 1.0);
const vec3 kGlowColor = vec3(0.55, 0.12, 0.95);
const vec3 kArcColor  = vec3(0.75, 0.55, 1.0);

// We hash integer cells because float sin hashes come out different on every driver
float Hash(ivec3 cell)
{
    uvec3 v = uvec3(cell) * uvec3(1597334673u, 3812015801u, 2798796415u);
    uint h  = (v.x ^ v.y ^ v.z) * 1597334673u;
    h ^= h >> 16;
    return float(h) / 4294967295.0;
}

float Noise(vec3 p)
{
    ivec3 i = ivec3(floor(p));
    vec3 f  = fract(p);
    f       = f * f * (3.0 - 2.0 * f);
    float a = mix(Hash(i), Hash(i + ivec3(1, 0, 0)), f.x);
    float b = mix(Hash(i + ivec3(0, 1, 0)), Hash(i + ivec3(1, 1, 0)), f.x);
    float c = mix(Hash(i + ivec3(0, 0, 1)), Hash(i + ivec3(1, 0, 1)), f.x);
    float d = mix(Hash(i + ivec3(0, 1, 1)), Hash(i + ivec3(1, 1, 1)), f.x);
    return mix(mix(a, b, f.y), mix(c, d, f.y), f.z);
}

// We hash integer cells because float sin hashes come out different on every driver
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

// The travel offset wraps by 256 cells, which is the period CellColor masks z to
float Walls(vec3 p, float travel, out vec3 color)
{
    float twist = p.z * 0.012;
    p.xy        = mat2(cos(twist), sin(twist), -sin(twist), cos(twist)) * p.xy;
    p.z += travel;
    color  = CellColor(ivec3(floor(p / 2.0 + 0.5)));
    p.yz   = mod(p.yz - 1.0, 2.0) - 1.0;
    p.x    = abs(p.x) - 28.0;
    vec3 q = abs(p) - vec3(0.7);
    return min(max(q.x, max(q.y, q.z)), 0.0) + length(max(q, 0.0)) - 0.05;
}

// We start from the scaled crack entry point so the space behind it shifts as the camera moves
vec3 Void(vec3 entry, vec3 direction, float seed, float time)
{
    mat2 spin    = mat2(cos(seed), -sin(seed), sin(seed), cos(seed));
    entry.xz     = spin * entry.xz;
    direction.xz = spin * direction.xz;
    float travel = mod(time * 6.0 + seed * 5.0, 512.0);
    vec3 origin  = vec3(0.0, 0.0, -15.0) + entry * 6.0;
    vec3 glow    = vec3(0.0);
    vec3 hitColor;
    float t     = 0.0;
    float steps = 1.0;
    for (int i = 1; i < kVoidSteps; i++)
    {
        steps      = float(i);
        float wall = abs(Walls(origin + direction * t, travel, hitColor)) - 0.001;
        glow += exp(-wall * 0.1) * hitColor * 0.5;
        // The twist stretches the distance field, so we take shorter steps to avoid marching through walls
        t += wall * 0.75;
        if (wall < 0.0001 || t > kVoidFar)
        {
            break;
        }
    }

    vec3 sky   = length(pow(abs(direction + vec3(0.0, 0.5, 0.0)), vec3(3.0))) * vec3(0.1, 0.0, 0.2) + glow / steps;
    vec3 color = t > kVoidFar ? sky : hitColor * 1.4 * exp(-t * 0.02) + sky;
    return color + vec3(0.75, 0.45, 1.0) * exp(-length(direction.xy) * 6.0) * 0.6;
}
