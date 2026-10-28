// We keep a ramp to full fog at the far end so the edge of loaded chunks stays hidden even in thin Nimbus air
float DistanceFog(float viewDistance)
{
    if (u_fogEnabled < 0.5)
    {
        return 0.0;
    }

    if (u_nimbusFog <= 0.0)
    {
        return clamp((viewDistance - u_fogRange.x) / max(u_fogRange.y - u_fogRange.x, 0.0001), 0.0, 1.0) * u_fogStrength;
    }

    float haze = 1.0 - exp(-viewDistance * u_nimbusFog * 1.2 / u_fogRange.y);
    return max(haze, smoothstep(0.85, 1.0, viewDistance / u_fogRange.y)) * u_fogStrength;
}
