#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_texture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_alpha;
};

layout(location = 0) out vec4 fragColor;

void main()
{
    fragColor = texture(u_texture, v_uv) * u_alpha;
}
