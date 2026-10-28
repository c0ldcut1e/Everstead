#version 450

// We remap GL depth to the Vulkan range here when the device has no depth clip control
layout(constant_id = 0) const bool kRemapDepth = false;

layout(location = 0) out vec2 v_ndc;

void main()
{
    vec2 p;
    if (gl_VertexIndex == 0)
    {
        p = vec2(-1.0, -1.0);
    }
    else if (gl_VertexIndex == 1)
    {
        p = vec2(3.0, -1.0);
    }
    else
    {
        p = vec2(-1.0, 3.0);
    }
    v_ndc       = p;
    gl_Position = vec4(p, 0.0, 1.0);
    if (kRemapDepth)
    {
        gl_Position.z = (gl_Position.z + gl_Position.w) * 0.5;
    }
}
