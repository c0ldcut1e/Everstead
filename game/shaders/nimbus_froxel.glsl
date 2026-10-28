// Slices grow exponentially with distance so near beams keep their detail and far ones stay cheap
float FroxelDepth(float slice)
{
    return slice <= 0.0 ? 0.0 : 0.5 * pow(u_maxDistance / 0.5, slice / u_froxelGrid.z);
}

// The froxel slices are laid out 8 to a row in the atlas
ivec2 FroxelTexel(ivec3 froxel)
{
    ivec2 size = ivec2(u_froxelGrid.xy);
    return ivec2((froxel.z & 7) * size.x + froxel.x, (froxel.z >> 3) * size.y + froxel.y);
}

ivec3 FroxelAt(ivec2 pixel)
{
    ivec2 size = ivec2(u_froxelGrid.xy);
    return ivec3(pixel % size, pixel.x / size.x + pixel.y / size.y * 8);
}

// We filter each slice in hardware and blend two slices ourselves, clamping inside the slice so it never reads its neighbor in the atlas
vec4 FroxelSample(sampler2D atlas, int slice, vec2 cell)
{
    return textureLod(atlas, (vec2(FroxelTexel(ivec3(0, 0, slice))) + cell) / vec2(textureSize(atlas, 0)), 0.0);
}
