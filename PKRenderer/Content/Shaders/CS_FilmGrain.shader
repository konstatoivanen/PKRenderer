#pragma pk_program SHADER_STAGE_COMPUTE main
#include "includes/Common.glsl"

PK_DECLARE_SET_DRAW uniform image2D pk_Image;

// Source: https://www.shadertoy.com/view/4sSXDW
// Changed 78.233f to 88.233f to fix visible banding.
float NoiseGrain0(const float2 n, const float x) 
{ 
    return fract(sin(dot(n.xy + x.xx, float2(12.9898f, 88.233f))) * 43758.5453f); 
}

float NoiseGrain1(const float2 n, const float x, const float range)
{
    const float b = 2.0, c = -12.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) *
    (
        NoiseGrain0(mod(n + float2(-1.0, -1.0), range.xx), x) +
        NoiseGrain0(mod(n + float2(+0.0, -1.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2(+1.0, -1.0), range.xx), x) +
        NoiseGrain0(mod(n + float2(-1.0, +0.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2(+0.0, +0.0), range.xx), x) * c +
        NoiseGrain0(mod(n + float2(+1.0, +0.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2(-1.0, +1.0), range.xx), x) +
        NoiseGrain0(mod(n + float2(+0.0, +1.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2(+1.0, +1.0), range.xx), x)
    );
}

float NoiseGrain2(const float2 n, const float x, const float range)
{
    const float b = 2.0, c = 4.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) *
    (
        NoiseGrain1(n + float2(-1.0, -1.0), x, range) +
        NoiseGrain1(n + float2(+0.0, -1.0), x, range) * b +
        NoiseGrain1(n + float2(+1.0, -1.0), x, range) +
        NoiseGrain1(n + float2(-1.0, +0.0), x, range) * b +
        NoiseGrain1(n + float2(+0.0, +0.0), x, range) * c +
        NoiseGrain1(n + float2(+1.0, +0.0), x, range) * b +
        NoiseGrain1(n + float2(-1.0, +1.0), x, range) +
        NoiseGrain1(n + float2(+0.0, +1.0), x, range) * b +
        NoiseGrain1(n + float2(+1.0, +1.0), x, range)
    );
}

float NoiseGrainBW(const float2 n, const float x, const float range) 
{ 
    return NoiseGrain2(n, fract(x), range) * 10.2f; 
}

float3 NoiseGrainColor(const float2 uv, const float x, const float range)
{
    float3 grain;
    grain.x = NoiseGrain2(uv, 0.07 * fract(x), range);
    grain.y = NoiseGrain2(uv + 17.0f.xx, 0.11 * fract(x), range);
    grain.z = NoiseGrain2(uv + 29.0f.xx, 0.13 * fract(x), range);
    grain *= 10.2f;
    return grain;
}

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    const float phase = make_unorm(pk_FrameRandom.x);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float3 grain = NoiseGrainColor(coord, phase, 256.0f);
    imageStore(pk_Image, coord, float4(grain, 1.0f));
}