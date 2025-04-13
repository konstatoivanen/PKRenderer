
#pragma pk_program SHADER_STAGE_COMPUTE main
#pragma pk_multi_compile _ VOLUME_FOG_CLEAR

#include "includes/VolumeFog.glsl"
#include "includes/Encoding.glsl"

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const int3 pos = int3(gl_GlobalInvocationID);
    const float3 dither = GlobalNoiseBlue(pos.xy, pk_FrameIndex.x);
    const float3 uvw_cur = (pos + dither) / VOLUMEFOG_SIZE;

    const float3 world_pos = UvToWorldPos(uvw_cur.xy, Fog_ZToView(uvw_cur.z));
    const float3 uvw_prev = Fog_WorldToPrevUvw(world_pos);

    const float value_cur = Fog_CalculateDensity(world_pos);

#if defined(VOLUME_FOG_CLEAR)
    const float value_pre = value_cur;
    const float3 inject_pre = 0.0f.xxx;
#else
    const float value_pre = SAMPLE_TRICUBIC(pk_Fog_DensityRead, uvw_prev).x;
    const float3 inject_pre = texelFetch(pk_Fog_InjectRead, pos, 0).rgb;
#endif

    const float value_out = lerp(value_pre, value_cur, Fog_GetAccumulation(uvw_prev));

    imageStore(pk_Fog_Density, pos, -min(0.0f, -value_out).xxxx);
    imageStore(pk_Fog_Inject, pos, EncodeE5BGR9(inject_pre).xxxx);
}