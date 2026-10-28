#version 450

layout(location = 0) in vec2 v_position;
layout(location = 1) in vec2 v_uv;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    vec4 u_rect;
    float u_radius;
    vec4 u_colorTop;
    vec4 u_colorBottom;
    vec4 u_borderColor;
    vec4 u_shadowColor;
    vec2 u_shadowOffset;
    float u_shadowBlur;
    float u_borderSize;
    float u_gloss;
};

layout(location = 0) out vec4 fragColor;

float RoundedRectDistance(vec2 point, vec2 center, vec2 halfSize, float radius)
{
    vec2 q = abs(point - center) - halfSize + vec2(radius);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

vec4 Over(vec4 top, vec4 bottom)
{
    float alpha = top.a + bottom.a * (1.0 - top.a);
    if (alpha <= 0.0001)
    {
        return vec4(0.0);
    }

    vec3 color = (top.rgb * top.a + bottom.rgb * bottom.a * (1.0 - top.a)) / alpha;
    return vec4(color, alpha);
}

void main()
{
    vec2 center   = u_rect.xy + u_rect.zw * 0.5;
    vec2 halfSize = u_rect.zw * 0.5;

    float shadowDistance = RoundedRectDistance(v_position - u_shadowOffset, center, halfSize, u_radius);
    float shadowAlpha    = smoothstep(u_shadowBlur, 0.0, shadowDistance) * u_shadowColor.a;

    float distanceValue = RoundedRectDistance(v_position, center, halfSize, u_radius);
    float fillAlpha     = smoothstep(1.5, -1.5, distanceValue);
    float borderAlpha   = smoothstep(u_borderSize + 1.5, u_borderSize - 1.5, abs(distanceValue));

    vec4 fillColor   = mix(u_colorTop, u_colorBottom, clamp(v_uv.y, 0.0, 1.0));
    float upperGloss = smoothstep(0.58, 0.0, v_uv.y) * smoothstep(0.0, 0.12, v_uv.y);
    fillColor.rgb += vec3(upperGloss * u_gloss);

    vec4 color = vec4(u_shadowColor.rgb, shadowAlpha * (1.0 - fillAlpha));
    color      = Over(vec4(fillColor.rgb, fillColor.a * fillAlpha), color);
    color      = Over(vec4(u_borderColor.rgb, u_borderColor.a * borderAlpha * fillAlpha), color);

    fragColor = color;
}
