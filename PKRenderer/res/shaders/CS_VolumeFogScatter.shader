#pragma PROGRAM_COMPUTE
#include includes/VolumeFog.glsl
#include includes/Encoding.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    float4 accum_scatter = float4(0.0f.xxx, 1.0f);
    float3 accum_transmittance = 1.0f.xxx;

    int3 pos = int3(gl_GlobalInvocationID.xy, 0);

    for (; pos.z < VOLUMEFOG_SIZE_Z; ++pos.z)
    {
        const float depthMin = VFog_ZToView(pos.z * VOLUMEFOG_SIZE_Z_INV);
        const float depthMax = VFog_ZToView((pos.z + 1.0f) * VOLUMEFOG_SIZE_Z_INV);
        const float slicewidth = depthMax - depthMin;

        const float  density = texelFetch(pk_Fog_DensityRead, pos, 0).x;
        const float3 irradiance = texelFetch(pk_Fog_InjectRead, pos, 0).rgb * pk_Fog_Albedo.rgb;

        const float  extinction = density * slicewidth;
        const float3 transmittance = exp(-extinction * pk_Fog_Absorption.rgb);
        const float3 integral = irradiance * (1.0f - transmittance) * accum_transmittance;

        accum_scatter.rgb += integral;
        // Store transmittance as it suffers less from fp16 reduction than extinction.
        accum_scatter.a *= exp(-extinction); 
        accum_transmittance *= transmittance;

        imageStore(pk_Fog_Scatter, pos, accum_scatter);

        // Copy previous values for reprojection
        // Iraddiance is copied in density pass to alleviate memory load of this pass
        imageStore(pk_Fog_Density, pos, density.xxxx);
    }
}