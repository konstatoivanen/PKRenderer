#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

float2 GetPreviousUV(const float depth, const int2 coord, const int2 size)
{
    float4 viewpos = float4(SampleViewPosition(coord, size, depth), 1.0f);
    return ClipToUVW(mul(pk_MATRIX_LD_P, viewpos)).xy;// -(pk_ProjectionJitter.zw * pk_ScreenParams.zw * 0.5f);
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

    const float2 uv = (coord + 0.5f.xx) / size;
    const float depth = SampleLinearDepth(uv);
    const float3 normal = SampleViewNormal(coord);
    const float2 prevUV = GetPreviousUV(depth, coord, size) * size - 0.5.xx;
    const int2 prevCoord = int2(prevUV);
    const float2 ddxy = prevUV - prevCoord;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };
    
    SH diffSH = pk_ZeroSH;
    SH specSH = pk_ZeroSH;
    SceneGIMeta meta = SceneGI_DecodeMeta(0u);

    meta.isOOB = !DepthFarCull(depth);

    if (meta.isOOB)
    {
        StoreGI_Meta(coord, meta);
        StoreGI_SH(coord, PK_GI_DIFF_LVL, diffSH);
        StoreGI_SH(coord, PK_GI_SPEC_LVL, specSH);
        return;
    }

    meta.isActive = All_Equal(coord % PK_GI_CHECKERBOARD_OFFSET, uint2(0u));

    float fHistory = 0.0f;

    float wSH = 0.0f;
    float wSum = 0.0f;

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        int2 xy = prevCoord + int2(xx, yy);
        const float weight = bilinearWeights[yy][xx];
        
        if (Any_Less(xy, int2(0)) || Any_GEqual(xy, size) || weight < 1e-4f)
        {
            continue;
        }

        const float depthPrev = SamplePreviousLinearDepth(xy);
        const float3 normalPrev = SamplePreviousViewNormal(xy);
        const float normalDot = dot(normalPrev, normal);

        if (!DepthReprojectCull(depth, depthPrev) || normalDot <= 0.05f)
        {
            continue;
        }

        diffSH = AddSH(diffSH, SampleGI_SH(xy, PK_GI_DIFF_LVL), weight * normalDot);
        specSH = AddSH(specSH, SampleGI_SH(xy, PK_GI_SPEC_LVL), weight * normalDot);
        
        SceneGIMeta sampleMeta = SceneGI_DecodeMeta(imageLoad(pk_ScreenGI_Meta_Read, xy).x);
        meta.moments += sampleMeta.moments * weight;
        fHistory += sampleMeta.history * weight;

        wSum += weight;
        wSH += weight * normalDot;
    }

    if (wSH > 1e-4f)
    {
        meta.moments /= wSum;
        meta.history = uint(fHistory / wSum) + 1;

        diffSH = ScaleSH(diffSH, 1.0f / wSH);
        specSH = ScaleSH(specSH, 1.0f / wSH);
    }

    if (IsNaN(diffSH.Y) || IsNaN(diffSH.CoCg) || IsNaN(specSH.Y) || IsNaN(specSH.CoCg))
    {
        diffSH = pk_ZeroSH;
        specSH = pk_ZeroSH;
    }

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, diffSH);
    StoreGI_SH(coord, PK_GI_SPEC_LVL, specSH);
}