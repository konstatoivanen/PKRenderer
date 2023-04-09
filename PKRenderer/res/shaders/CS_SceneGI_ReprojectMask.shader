#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    int2 size = imageSize(pk_ScreenGI_Mask).xy;
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    float2 uv = (coord + 0.5f.xx) / size;

    float depthCurrent = SampleLinearDepth(uv);
    float4 viewpos = float4(SampleViewPosition(coord, size, depthCurrent), 1.0f);
    float3 worldpos = mul(pk_MATRIX_I_V, viewpos).xyz;
    float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, viewpos)) - float3(pk_ProjectionJitter.zw * pk_ScreenParams.zw * 0.5f, 0.0f);
    float depthPrevious = SamplePreviousLinearDepth(uvw.xy);
    float depthDelta = abs(depthCurrent - depthPrevious) / max(max(depthCurrent, depthPrevious), 1e-4f);

    GIMask mask;
    GIMask prevMask = LoadGIMask(coord);

    bool hasDiscontinuity = depthDelta > 0.1f || Any_Greater(abs(uvw.xy - 0.5f), 0.5f.xx);
    mask.discontinuityFrames = hasDiscontinuity ? 8u : uint(max(0, int(prevMask.discontinuityFrames) - 1));
    mask.isActive = All_Equal(coord % PK_GI_CHECKERBOARD_OFFSET, uint2(0u));
    mask.isOOB = Any_Greater(abs(WorldToVoxelClipSpace(worldpos)), 1.0f.xxx);

    StoreGIMask(coord, mask);

    SH diffSH = SampleGI_SH(uvw.xy, PK_GI_DIFF_LVL);
    SH specSH = SampleGI_SH(uvw.xy, PK_GI_SPEC_LVL);

    //Remove directional SH component based on normal similarity to reduce sampling error.
    const float3 nref = SampleViewNormal(coord);
    const float3 nsmp = SamplePreviousViewNormal(uvw.xy);
    const float ndot = max(0.0f, dot(nref, nsmp));
    diffSH.SHY.yzw *= ndot;
    specSH.SHY.yzw *= ndot;

    StoreGI_SH(coord, PK_GI_DIFF_LVL, diffSH);
    StoreGI_SH(coord, PK_GI_SPEC_LVL, specSH);
}