#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_currentColor;
layout(set = 1, binding = 1) uniform sampler2D u_currentDepth;
layout(set = 1, binding = 2) uniform sampler2D u_historyColor;
layout(set = 1, binding = 3) uniform sampler2D u_historyDepth;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    float u_hasHistory;
    float u_historyWeight;
    float u_depthTolerance;
    float u_depthFadeRange;
};

layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 currentColor  = texture(u_currentColor, v_uv);
    float currentDepth = texture(u_currentDepth, v_uv).r;
    if (currentDepth >= 0.99999 || currentColor.a <= 0.0001)
    {
        FragColor = vec4(0.0);
        return;
    }

    vec3 color = currentColor.rgb;

    if (u_hasHistory > 0.5 && u_historyWeight > 0.0001)
    {
        vec4 historyColor  = texture(u_historyColor, v_uv);
        float historyDepth = texture(u_historyDepth, v_uv).r;
        if (historyDepth < 0.99999 && historyColor.a > 0.0001)
        {
            float depthDelta    = abs(currentDepth - historyDepth);
            float depthMatch    = 1.0 - smoothstep(u_depthTolerance, u_depthTolerance + u_depthFadeRange, depthDelta);
            float historyFactor = clamp(u_historyWeight * depthMatch * historyColor.a, 0.0, 0.95);
            color               = mix(color, historyColor.rgb, historyFactor);
        }
    }

    FragColor = vec4(color, currentColor.a);
}
