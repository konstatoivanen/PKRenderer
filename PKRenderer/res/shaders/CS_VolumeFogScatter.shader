#version 460
#pragma PROGRAM_COMPUTE
#include includes/SharedVolumeFog.glsl
#include includes/Encoding.glsl

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    float3 accum_irradiance = 0.0f.xxx;
    float3 accum_transmittance = 1.0f.xxx;

    int3 pos = int3(gl_GlobalInvocationID.xy, 0);

    #pragma unroll VOLUMEFOG_SIZE_Z
    for (; pos.z < VOLUMEFOG_SIZE_Z; ++pos.z)
    {
        const float2 depths = ViewDepthExp(float2(pos.z, pos.z + 1.0f) * VOLUMEFOG_SIZE_Z_INV);
        const float slicewidth = depths.y - depths.x;

        const float  density = texelFetch(pk_Fog_DensityRead, pos, 0).x;
        const float3 irradiance = texelFetch(pk_Fog_InjectRead, pos, 0).rgb * pk_Fog_Albedo.rgb;
        const float3 transmittance = exp(-density * pk_Fog_Absorption.rgb * slicewidth);
        const float3 integral = irradiance * (1.0f - transmittance);

        accum_irradiance += integral * accum_transmittance;
        accum_transmittance *= transmittance;

        imageStore(pk_Fog_Scatter, pos, uint4(EncodeE5BGR9(accum_irradiance)));
        imageStore(pk_Fog_Transmittance, pos, float4(accum_transmittance, 1.0f));
        
        // Copy previous values for reprojection
        // Iraddiance is copied in density pass to alleviate memory load of this pass
        imageStore(pk_Fog_Density, pos, density.xxxx);
    }
}