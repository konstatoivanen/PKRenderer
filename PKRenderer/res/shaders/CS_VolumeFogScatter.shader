#version 460
#pragma PROGRAM_COMPUTE
#include includes/SharedVolumeFog.glsl

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const float2 uv = (gl_GlobalInvocationID.xy + 0.5f.xx) / VOLUME_SIZE_XY;
    const float3 vpos = ClipUVToViewPos(uv, 1.0f);

    float4 accumulation = float4(0, 0, 0, 1);
    int3 pos = int3(gl_GlobalInvocationID.xy, 0);

#pragma unroll VOLUME_DEPTH
    for (; pos.z < VOLUME_DEPTH; ++pos.z)
    {
        float2 depths = GetVolumeCellDepth(float2(pos.z, pos.z + 1.0f));
        float depth = lerp(depths.x, depths.y, 0.5f);
        float slicewidth = depths.y - depths.x;

        float4 slice = imageLoad(pk_Volume_Inject, pos);

        float  transmittance = exp(-slice.a * slicewidth);
        float3 lightintegral = slice.rgb * (1.0f - transmittance) / slice.a;

        accumulation.rgb += lightintegral * accumulation.a;
        accumulation.a *= transmittance;

        float4 preval = tex2D(pk_Volume_ScatterRead, ReprojectViewToCoord(vpos * depth));
        float4 outval = lerp(preval, accumulation, VOLUME_ACCUMULATION);

        imageStore(pk_Volume_Scatter, pos, outval);
    }
}