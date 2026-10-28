#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_colorTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_gammaExponent;
};

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 color = texture(u_colorTexture, v_uv);
    vec3 rgb   = pow(max(color.rgb, vec3(0.0)), vec3(max(u_gammaExponent, 0.0001)));
    FragColor  = vec4(clamp(rgb, 0.0, 1.0), color.a);
}
