#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

SceneGIMeta SampleGI_Meta_Bilinear(float2 uv, const int2 size)
{
    uv = uv * size - 0.5f.xx;
    const int2 coord = int2(uv);
    const float2 ddxy = uv - coord;

    float4 w;
    w.x = (1.0f - ddxy.x) * (1.0f - ddxy.y);
    w.y = ddxy.x * (1.0f - ddxy.y);
    w.z = (1.0f - ddxy.x) * ddxy.y;
    w.w = ddxy.x * ddxy.y;

    SceneGIMeta m00 = SceneGI_DecodeMeta(imageLoad(pk_ScreenGI_Meta_Read, coord + int2(0, 0)).x);
    SceneGIMeta m10 = SceneGI_DecodeMeta(imageLoad(pk_ScreenGI_Meta_Read, coord + int2(1, 0)).x);
    SceneGIMeta m01 = SceneGI_DecodeMeta(imageLoad(pk_ScreenGI_Meta_Read, coord + int2(0, 1)).x);
    SceneGIMeta m11 = SceneGI_DecodeMeta(imageLoad(pk_ScreenGI_Meta_Read, coord + int2(1, 1)).x);

    m00.mindist = m00.mindist * w.x + m10.mindist * w.y + m01.mindist * w.z + m11.mindist * w.w;
    m00.history = uint(m00.history * w.x + m10.history * w.y + m01.history * w.z + m11.history * w.w);
    return m00;
}

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 size = int2(pk_ScreenSize.xy);
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    float2 uv = (coord + 0.5f.xx) / size;

    float currentZ = SampleLinearDepth(uv);
    float4 viewpos = float4(SampleViewPosition(coord, size, currentZ), 1.0f);
    float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, viewpos)) - float3(pk_ProjectionJitter.zw * pk_ScreenParams.zw * 0.5f, 0.0f);
    float previousZ = SamplePreviousLinearDepth(uvw.xy);
    bool hasDiscontinuity = !DepthReprojectCull(currentZ, previousZ) || Any_Greater(abs(uvw.xy - 0.5f), 0.5f.xx);

    SceneGIMeta meta = SampleGI_Meta_Bilinear(uvw.xy, size);
    meta.history = min(PK_GI_MAX_HISTORY, hasDiscontinuity ? 0u : (meta.history + 1));
    meta.mindist = hasDiscontinuity ? 1.0f : meta.mindist;
    meta.isActive = All_Equal(coord % PK_GI_CHECKERBOARD_OFFSET, uint2(0u));
    meta.isOOB = currentZ > (pk_ProjectionParams.z - 1e-2f);

    SH diffSH = SampleGI_SH(uvw.xy, PK_GI_DIFF_LVL);
    SH specSH = SampleGI_SH(uvw.xy, PK_GI_SPEC_LVL);
    
    //Remove directional SH component based on normal similarity to reduce sampling error.
    const float3 currentN = SampleViewNormal(coord);
    const float3 previousN = SamplePreviousViewNormal(uvw.xy);
    const float normalDot = max(0.0f, dot(currentN, previousN));
    diffSH = DegenerateSH(diffSH, normalDot);
    specSH = DegenerateSH(specSH, normalDot);

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, diffSH);
    StoreGI_SH(coord, PK_GI_SPEC_LVL, specSH);
}