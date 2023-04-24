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
    const float depth = SampleLinearDepth(coord);
    const float3 vnormal = SampleViewNormal(coord);
    const float depthBias = lerp(0.1f, 0.01f, -vnormal.z);
    const float3 normal = mul(float3x3(pk_MATRIX_I_V), vnormal);
    const float2 uvPrev = GetPreviousUV(depth, coord, size) * size - 0.5.xx;
    const int2 coordPrev = int2(uvPrev);
    const float2 ddxy = uvPrev - coordPrev;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };
    
    SH diffSH = pk_ZeroSH;
    SH specSH = pk_ZeroSH;
    SceneGIMeta meta = SceneGI_DecodeMeta(0u);
   // meta.isActive = All_Equal(coord % PK_GI_CHECKERBOARD_OFFSET, uint2(0u));

    if (!Test_DepthFar(depth))
    {
        StoreGI_Meta(coord, meta);
        StoreGI_SH(coord, PK_GI_DIFF_LVL, diffSH);
        StoreGI_SH(coord, PK_GI_SPEC_LVL, specSH);
        return;
    }


    float fHistory = 0.0f;
    float wSum = 0.0f;

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        const int2 xy = coordPrev + int2(xx, yy);
        const float weight = bilinearWeights[yy][xx];
        
        if (!All_InArea(xy, int2(0), size) || weight < 1e-4f)
        {
            continue;
        }

        const float depthPrev = SamplePreviousLinearDepth(xy);
        const float3 normalPrev = SamplePreviousViewNormal(xy);
        const float normalDot = dot(normalPrev, vnormal);

        if (!Test_DepthReproject(depth, depthPrev, depthBias) || normalDot <= 0.05f)
        {
            continue;
        }

        diffSH = AddSH(diffSH, SampleGI_SH(xy, PK_GI_DIFF_LVL), weight * normalDot);
        specSH = AddSH(specSH, SampleGI_SH(xy, PK_GI_SPEC_LVL), weight * normalDot);
        
        SceneGIMeta sampleMeta = SceneGI_DecodeMeta(imageLoad(pk_ScreenGI_Meta_Read, xy).x);
        meta.moments += sampleMeta.moments * weight * normalDot;
        fHistory += sampleMeta.history * weight * normalDot;
        wSum += weight * normalDot;
    }

    if (wSum > 1e-4f)
    {
        meta.moments /= wSum;
        meta.history = uint(round(fHistory / wSum)) + 1;
        diffSH = ScaleSH(diffSH, 1.0f / wSum);
        specSH = ScaleSH(specSH, 1.0f / wSum);
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