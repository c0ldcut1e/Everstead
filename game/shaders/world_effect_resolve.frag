#version 450

layout(location = 0) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_currentTexture;
layout(set = 1, binding = 1) uniform sampler2D u_historyTexture;
layout(set = 1, binding = 2) uniform sampler2D u_depthTexture;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invViewProj;
    mat4 u_reproject;
    float u_focusDistance;
    float u_historyWeight;
};

layout(location = 0) out vec4 FragColor;

void main()
{
    ivec2 texel     = ivec2(gl_FragCoord.xy);
    ivec2 maxTexel  = textureSize(u_currentTexture, 0) - 1;
    vec4 current    = texelFetch(u_currentTexture, texel, 0);
    vec4 lowerBound = current;
    vec4 upperBound = current;
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            vec4 neighbor = texelFetch(u_currentTexture, clamp(texel + ivec2(x, y), ivec2(0), maxTexel), 0);
            lowerBound    = min(lowerBound, neighbor);
            upperBound    = max(upperBound, neighbor);
        }
    }

    if (u_historyWeight <= 0.0)
    {
        FragColor = current;
        return;
    }

    // Effects float in the air, so we reproject at the nearest effect unless the scene is closer
    vec2 ndc       = v_uv * 2.0 - 1.0;
    vec4 nearPos   = u_invViewProj * vec4(ndc, -1.0, 1.0);
    vec4 farPos    = u_invViewProj * vec4(ndc, 1.0, 1.0);
    vec4 hitPos    = u_invViewProj * vec4(ndc, texture(u_depthTexture, v_uv).r * 2.0 - 1.0, 1.0);
    vec3 ro        = nearPos.xyz / nearPos.w;
    vec3 rd        = normalize(farPos.xyz / farPos.w - ro);
    float t        = min(length(hitPos.xyz / hitPos.w - ro), u_focusDistance);
    vec4 prevClip  = u_reproject * vec4(ro + rd * t, 1.0);
    vec2 historyUv = prevClip.xy / prevClip.w * 0.5 + 0.5;
    if (prevClip.w <= 0.0 || any(lessThan(historyUv, vec2(0.0))) || any(greaterThan(historyUv, vec2(1.0))))
    {
        FragColor = current;
        return;
    }

    // Clamping history to this frame's neighborhood keeps strobes and moving shells from leaving trails
    vec4 history = clamp(texture(u_historyTexture, historyUv), lowerBound, upperBound);
    FragColor    = mix(current, history, u_historyWeight);
}
