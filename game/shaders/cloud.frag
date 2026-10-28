#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;
layout(location = 2) in float v_viewZ;

layout(set = 1, binding = 0) uniform sampler2D u_texture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_alphaCutoff;
    float u_fogEnabled;
    vec3 u_fogColor;
    vec2 u_fogRange;
};

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 textureColor = texture(u_texture, v_uv);
    if (textureColor.a <= u_alphaCutoff)
    {
        discard;
    }

    vec4 color = v_color * textureColor;

    if (u_fogEnabled > 0.5)
    {
        float fogDistance = abs(v_viewZ);
        float fogFactor = clamp((fogDistance - u_fogRange.x) / max(u_fogRange.y - u_fogRange.x, 0.0001), 0.0, 1.0);
        color.rgb = mix(color.rgb, u_fogColor, fogFactor);
    }

    FragColor = color;
}
