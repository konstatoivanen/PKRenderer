#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

layout(local_size_x = 16, local_size_y = 2, local_size_z = 1) in;
void main()
{
    int2 size = int2(pk_ScreenSize.xy);
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    SH diffSH = pk_ZeroSH;
    SH specSH = pk_ZeroSH;

    const float2 uv = (coord + 0.5f.xx) / size;
    const float depth = SampleLinearDepth(coord);
    const float3 normal = SampleViewNormal(coord);
    const float depthBias = lerp(0.1f, 0.01f, -normal.z);
    const float4 viewpos = float4(SampleViewPosition(coord, size, depth), 1.0f);
    const float2 uvPrev = ClipToUVW(mul(pk_MATRIX_LD_P, viewpos)).xy * size - 0.5.xx;
    const int2 coordPrev = int2(uvPrev);
    const float2 ddxy = uvPrev - coordPrev;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };
    
    if (!Test_DepthFar(depth))
    {
        GI_Store_SH(coord, PK_GI_DIFF_LVL, diffSH);
        GI_Store_SH(coord, PK_GI_SPEC_LVL, specSH);
        return;
    }

    float2 sumMoments = 0.0f.xx;
    float sumHistory = 0.0f;
    float wSum = 0.0f;

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        const int2 xy = coordPrev + int2(xx, yy);
        float weight = bilinearWeights[yy][xx];
        
        if (!All_InArea(xy, int2(0), size) || weight < 1e-4f)
        {
            continue;
        }

        const float depthPrev = SamplePreviousLinearDepth(xy);

        if (!Test_DepthFar(depthPrev))
        {
            continue;
        }

        const float3 normalPrev = SamplePreviousViewNormal(xy);
        const float normalDot = dot(normalPrev, normal);

        if (!Test_DepthReproject(depth, depthPrev, depthBias) || normalDot <= 0.05f)
        {
            continue;
        }
     
        weight *= normalDot;
        diffSH = SHAdd(diffSH, GI_Load_SH(xy, PK_GI_DIFF_LVL), weight);
        specSH = SHAdd(specSH, GI_Load_SH(xy, PK_GI_SPEC_LVL), weight);
        sumMoments += GI_Load_Moments(xy) * weight;
        sumHistory += GI_Load_HistoryVariance(xy).x * weight;
        wSum += weight;
    }

    // Try to find valid samples with a bilateral cross filter
    if (wSum <= 1e-4f)
    {
        wSum = 0.0f;

        for (int yy = -1; yy <= 1; yy++)
        for (int xx = -1; xx <= 1; xx++)
        {
            const int2 xy = coordPrev + int2(xx, yy);

            if (!All_InArea(xy, int2(0), size))
            {
                continue;
            }

            const float depthPrev = SamplePreviousLinearDepth(xy);

            if (!Test_DepthFar(depthPrev))
            {
                continue;
            }

            const float3 normalPrev = SamplePreviousViewNormal(xy);
            const float normalDot = dot(normalPrev, normal);

            if (Test_DepthReproject(depth, depthPrev, depthBias) && normalDot > 0.05f)
            {
                diffSH = SHAdd(diffSH, GI_Load_SH(xy, PK_GI_DIFF_LVL), 1.0f);
                specSH = SHAdd(specSH, GI_Load_SH(xy, PK_GI_SPEC_LVL), 1.0f);
                sumMoments += GI_Load_Moments(xy);
                sumHistory += GI_Load_HistoryVariance(xy).x;
                wSum += 1.0f;
            }
        }
    }

    if (wSum > 1e-4f)
    {
        sumMoments /= wSum;
        sumHistory = (sumHistory / wSum) + 1;
        diffSH = SHScale(diffSH, 1.0f / wSum);
        specSH = SHScale(specSH, 1.0f / wSum);
    }

    if (IsNaN(diffSH.Y) || IsNaN(diffSH.CoCg) || IsNaN(specSH.Y) || IsNaN(specSH.CoCg))
    {
        diffSH = pk_ZeroSH;
        specSH = pk_ZeroSH;
    }

    GI_Store_Moments(coord, sumMoments);
    GI_Store_HistoryVariance(coord, float2(sumHistory, 0.0f));
    GI_Store_SH(coord, PK_GI_DIFF_LVL, diffSH);
    GI_Store_SH(coord, PK_GI_SPEC_LVL, specSH);
}