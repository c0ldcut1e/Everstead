#version 450

layout(location = 0) in vec2 v_ndc;

layout(std140, set = 0, binding = 1) uniform FragmentParams
{
    mat4 u_invProj;
    mat4 u_invViewRot;
    float u_fogEnabled;
    vec2 u_fogLut;
    vec3 u_skyLut;
    float u_celestialAngle;
    vec3 u_sunDir;
    float u_dayFactor;
};

layout(set = 1, binding = 0) uniform sampler2D u_skyColormap;
layout(set = 1, binding = 1) uniform sampler2D u_fogColormap;

layout(location = 0) out vec4 FragColor;

float Sat(float x)
{
    return clamp(x, 0.0, 1.0);
}

vec3 Sample0(sampler2D map, vec2 uv)
{
    return textureLod(map, clamp(uv, 0.0, 1.0), 0.0).rgb;
}

void main()
{
    vec4 v       = u_invProj * vec4(v_ndc, 1.0, 1.0);
    vec3 dirView = normalize(v.xyz / max(v.w, 1e-6));
    vec3 dir     = normalize(mat3(u_invViewRot) * dirView);

    float td = u_celestialAngle;
    float br = Sat(u_dayFactor);

    float skyLutX = Sat(u_skyLut.x);
    vec3 sky      = Sample0(u_skyColormap, vec2(skyLutX, 0.5)) * br;

    vec3 fogCol = Sample0(u_fogColormap, u_fogLut);
    fogCol.r *= br * 0.94 + 0.06;
    fogCol.g *= br * 0.94 + 0.06;
    fogCol.b *= br * 0.91 + 0.09;

    float up01      = Sat(dir.y * 0.5 + 0.5);
    float horizon01 = 1.0 - up01;
    float haze      = pow(smoothstep(0.0, 1.0, horizon01), 0.45);
    if (u_fogEnabled < 0.5)
    {
        haze = 0.0;
    }
    vec3 col = mix(sky, fogCol, haze);

    float span       = 0.4;
    float tt         = cos(td * 6.28318530718) - 0.0;
    float aa         = 0.0;
    float sunriseMix = 0.0;
    if (tt >= -span && tt <= span)
    {
        aa         = (tt / span) * 0.5 + 0.5;
        sunriseMix = 1.0 - ((1.0 - sin(aa * 3.14159265359)) * 0.99);
        sunriseMix *= sunriseMix;
    }

    vec3 sunriseDark    = Sample0(u_skyColormap, vec2(skyLutX, Sat(u_skyLut.y)));
    vec3 sunriseBright  = Sample0(u_skyColormap, vec2(skyLutX, Sat(u_skyLut.z)));
    vec3 sunriseMapCol  = mix(sunriseDark, sunriseBright, aa);
    vec3 sunriseBetaCol = vec3(aa * 0.3 + 0.7, aa * aa * 0.7 + 0.2, aa * aa * 0.0 + 0.2);
    vec3 sunriseCol     = mix(sunriseMapCol, sunriseBetaCol, 0.85);

    float sunFacing     = Sat(dot(normalize(u_sunDir), dir));
    float dirXZLen      = length(dir.xz);
    float horizonSide   = sin(td * 6.28318530718) < 0.0 ? -1.0 : 1.0;
    float azimuthFacing = 0.0;
    if (dirXZLen > 0.0001)
    {
        azimuthFacing = Sat(dot(dir.xz / dirXZLen, vec2(horizonSide, 0.0)));
    }
    float sunriseBand = exp(-pow((dir.y - 0.015) / 0.14, 2.0));
    float sunriseMask = sunriseMix * sunriseBand * pow(azimuthFacing, 1.9);
    float sunriseCore = sunriseMix * pow(sunFacing, 3.8) * exp(-pow((dir.y - u_sunDir.y) / 0.24, 2.0));
    col               = mix(col, sunriseCol, Sat(sunriseMask * 1.35 + sunriseCore * 0.5));

    float skyLuma        = (sky.r + sky.r + sky.b + sky.g + sky.g + sky.g) / 6.0;
    float ringBrightness = 0.6 + skyLuma * 0.4;
    float ringBand       = exp(-pow(abs(dir.y) / 0.055, 2.0));
    float ringHeight     = smoothstep(-0.08, 0.35, dir.y);
    float ringAzimuth    = mix(0.15, 1.0, pow(azimuthFacing, 2.0));
    float ringAlpha      = ringBand * ringHeight * ringAzimuth;
    if (u_fogEnabled < 0.5)
    {
        ringAlpha *= 0.35;
    }
    vec3 ringCol = mix(vec3(ringBrightness), sunriseCol, sunriseMix * ringAzimuth);
    col += ringCol * ringAlpha * 0.10;

    vec3 lowerCol   = vec3(sky.r * 0.2 + 0.04, sky.g * 0.2 + 0.04, sky.b * 0.6 + 0.1);
    float below01   = Sat(-dir.y);
    float voidBlend = smoothstep(0.04, 0.18, below01);
    float deep      = smoothstep(0.50, 1.0, below01);
    col             = mix(col, lowerCol, voidBlend);
    col             = mix(col, vec3(0.0), deep);

    FragColor = vec4(col, 1.0);
}
