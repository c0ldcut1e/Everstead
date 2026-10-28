#version 450

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(set = 1, binding = 0) uniform sampler2D u_font;

layout(location = 0) out vec4 fragColor;

void main()
{
    vec4 tex = texture(u_font, v_uv);
    if (tex.a <= 0.0)
    {
        discard;
    }

    fragColor = vec4(v_color.rgb, tex.a * v_color.a);
}
