#version 460

#multi_compile PASS_VARIANCE PASS_DISKBLUR

#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/Kernels.glsl

const int2 kernel[8] =
{
    int2(-1,  0),
    int2(-1,  1),
    int2( 0,  1),
             
    int2( 1,  1),
    int2( 1,  0),
    int2( 1, -1),
             
    int2( 0, -1),
    int2(-1, -1)
};

void DenoiseVariance(const int2 coord, const int2 size)
{
    SceneGIMeta meta = SampleGI_Meta(coord);

    if (meta.isOOB)
    {
        return;
    }

    float2 W = meta.history == 0u ? 0.125f.xx : 1.0f.xx;
    SH diffSH = ScaleSH(SampleGI_SH(coord, PK_GI_DIFF_LVL), W.x);
    SH specSH = ScaleSH(SampleGI_SH(coord, PK_GI_SPEC_LVL), W.y);

    const float D = SampleLinearDepth(coord);
    const float3 O = SampleViewPosition(coord, size, D);
    const float4 NR = SampleViewNormalRoughness(coord);
    float L = SHToAmbientLuminance(diffSH);
    float2 moments = float2(L, L * L) * W.x;
    
    const float scalePx = pk_ScreenParams.w * 2.0f / pk_MATRIX_P[1][1];
    const float scaleDepth = (pk_ProjectionParams.x + D.x);
    const float thresholdH = scalePx * scaleDepth * 16.0f;
    const float thresholdV = thresholdH * 0.5f;
    
    int kernelScale0 = clamp(int(meta.history), 1, 2);
    int kernelScale1 = clamp(int(meta.history) - 1, 1, 2);
    int swizzle = (coord.x + (coord.y % 2)) % 2;

    for (int i = 0; i < 8; ++i)
    {
        int2 offset = kernel[i];
        offset *= 1 + ((i + swizzle) % 2) * kernelScale0;
        offset *= kernelScale1;

        const int2 scoord = coord + offset;

        if (Any_Less(scoord, int2(0)) || Any_GEqual(scoord, size))
        {
            continue;
        }

        SH sampleDiff = SampleGI_SH(scoord, PK_GI_DIFF_LVL);
        SH sampleSpec = SampleGI_SH(scoord, PK_GI_SPEC_LVL);
        SceneGIMeta sampleMeta = SampleGI_Meta(scoord);

        const float3 sampleV = SampleViewPosition(scoord, size) - O;
        const float4 sampleNR = SampleViewNormalRoughness(scoord);
        const float sampleDistV = abs(dot(sampleV, NR.xyz));
        const float sampleDistH = length(sampleV);
        const float sampleDot = dot(NR.xyz, sampleNR.xyz);

        if (sampleDistV > thresholdV || sampleDistH > thresholdH || sampleDot <= 0.0)
        {
            continue;
        }

        const float sampleL = SHToAmbientLuminance(sampleDiff);

        const float weightH = sampleMeta.history == 0u ? 0.125f : 1.0f;
        const float weightP = max(0.0f, 1.0f - sampleDistV / thresholdV);
        const float weightD = max(0.0f, 1.0f - sampleDistH / thresholdH);
        const float weightN = pow(sampleDot, 256.0);
        const float weightR = pow4(1.0f - abs(NR.w - sampleNR.w));
        const float weightS = saturate(weightH * weightP * weightD * weightN);

        L = min(L, lerp(L, sampleL, weightS));
        moments += float2(sampleL, sampleL * sampleL) * weightS;

        W += float2(weightS, weightS * weightR);
        diffSH = AddSH(diffSH, sampleDiff, weightS);
        specSH = AddSH(specSH, sampleSpec, weightS * weightR);
    }

    moments /= W.x;

    const float diffDist = min(1.0f, imageLoad(pk_ScreenGI_Hits, coord).r / 5.0f);
    meta.mindist = lerp(meta.mindist, diffDist, diffDist < meta.mindist ? 0.25f : 0.025f);
    
    meta.variance = moments.y - moments.x * moments.x;
    meta.variance = meta.variance / (L * L);
    meta.variance /= lerp(0.25f, 1.0f, meta.history01);

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(diffSH, 1.0f / W.x));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(specSH, 1.0f / W.y));
}

void DenoiseDisk(const int2 coord, const int2 size)
{
    SceneGIMeta meta = SampleGI_Meta(coord);

    if (meta.isOOB)
    {
        return;
    }

    const float2 D = float2(SampleLinearDepth(coord), 1.0f - (SampleLinearDepth(coord) / pk_ProjectionParams.z));
    const float3 O = SampleViewPosition(coord, size, D.x);
    const float4 NR = SampleViewNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3x3 TBN = ComposeTBN(N);
    const float2 rotation = make_rotation(pk_FrameIndex * (PK_PI / 3.0f));

    const float normalPower = 0.25f + meta.history * 24.0f * D.y;
    const float scalePx = pk_ScreenParams.w * 2.0f / pk_MATRIX_P[1][1];
    const float scaleHit = lerp(2.0f, 64.0f, meta.mindist);
    const float scaleDepth = (pk_ProjectionParams.x + D.x);
    const float scaleVariance = meta.variance * 4.0f + 0.5f;
 
    const float thresholdH = scalePx * scaleHit * scaleDepth * scaleVariance;
    const float thresholdV = thresholdH * 0.5f;

    float3 clipuvw;

    float2 W = 1.0f.xx;
    SH diffSH = SampleGI_SH(coord, PK_GI_DIFF_LVL);
    SH specSH = SampleGI_SH(coord, PK_GI_SPEC_LVL);
    
    for (uint i = 0u; i < 16u; ++i)
    {
        const float3 samplePos = O + TBN * float3(rotate2D(PK_POISSON_DISK_POW2_16[i] * thresholdH, rotation), 0.0f);
    
        if (!TryGetViewToClipUVW(samplePos, clipuvw))
        {
            continue;
        }
    
        const int2 scoord = int2(clipuvw.xy * size);
        const float4 sampleNR = SampleViewNormalRoughness(scoord);
        const float3 sampleV = SampleViewPosition(scoord, size) - O;
        const float sampleDistV = abs(dot(sampleV, N));
        const float sampleDistH = dot(sampleV, sampleV);
        const float sampleDot = dot(N, sampleNR.xyz);
    
        if (sampleDistV > thresholdV || sampleDistH > (thresholdH * thresholdH) || sampleDot < 0)
        {
            continue;
        }
    
        const float weightN = pow(sampleDot, normalPower);
        const float weightP = 1.0f - (sampleDistV / thresholdV);
        const float weightD = 1.0f - sqrt(sampleDistH) / thresholdH;
        const float weightR = pow4(1.0f - abs(sampleNR.w - NR.w));
        const float weightS = saturate(weightN * weightP * weightD);
    
        SH sampleDiff = SampleGI_SH(scoord, PK_GI_DIFF_LVL);
        SH sampleSpec = SampleGI_SH(scoord, PK_GI_SPEC_LVL);
        sampleDiff = DegenerateSH(sampleDiff, sampleDot);
        sampleSpec = DegenerateSH(sampleSpec, sampleDot);
        diffSH = AddSH(diffSH, sampleDiff, weightS);
        specSH = AddSH(specSH, sampleSpec, weightS * weightR);
        W += float2(weightS, weightS * weightR);
    }

    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(diffSH, 1.0f / W.x));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(specSH, 1.0f / W.y));
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

#if defined(PASS_VARIANCE)
    DenoiseVariance(coord, size);
#else
    DenoiseDisk(coord, size);
#endif
}