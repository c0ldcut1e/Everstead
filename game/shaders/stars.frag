#version 450

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_starAlpha;
};

layout(location = 0) out vec4 FragColor;

float Sat(float x)
{
    return clamp(x, 0.0, 1.0);
}

void main()
{
    float a   = Sat(u_starAlpha);
    FragColor = vec4(1.0, 1.0, 1.0, a);
}
