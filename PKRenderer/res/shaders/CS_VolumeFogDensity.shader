#version 460
#pragma PROGRAM_COMPUTE
#include includes/VolumeFog.glsl
#include includes/Encoding.glsl

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const int3 pos = int3(gl_GlobalInvocationID);
    const float3 dither = GlobalNoiseBlue(pos.xy, pk_FrameIndex.x);
    const float3 uvw_cur = (pos + dither) / VOLUMEFOG_SIZE;

    const float3 worldpos = UVToWorldPos(uvw_cur.xy, ViewDepthExp(uvw_cur.z));
    const float3 uvw_prev = VolumeFog_WorldToPrevUVW(worldpos);

    const float accumulation = VolumeFog_GetAccumulation(uvw_prev);
    const float value_pre = ReplaceIfResized(SAMPLE_TRICUBIC(pk_Fog_DensityRead, uvw_prev).x, 0.0f);
    const float value_cur = VolumeFog_CalculateDensity(worldpos);
    const float value_out = lerp(value_pre, value_cur, accumulation);

    imageStore(pk_Fog_Density, pos, isnan(value_out) ? 0.0f.xxxx : value_out.xxxx);
    imageStore(pk_Fog_Inject, pos, EncodeE5BGR9(texelFetch(pk_Fog_InjectRead, pos, 0).rgb).xxxx);
}