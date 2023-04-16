#version 460

#multi_compile PASS_VARIANCE PASS_DISKBLUR

#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/Kernels.glsl

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
    const float4 NR = SampleWorldNormalRoughness(coord);
    float L = sqrt(SHToAmbientLuminance(diffSH));
    float2 moments = float2(L, L * L) * W.x;
    
    for (int x = -1; x <= 1; ++x)
    for (int y = -1; y <= 1; ++y)
    {
        const int2 scoord = coord + int2(x, y);

        if ((x == 0 && y == 0) || 
            Any_Less(scoord, int2(0)) || 
            Any_GEqual(scoord, size))
        {
            continue;
        }

        SH sampleDiff = SampleGI_SH(scoord, PK_GI_DIFF_LVL);
        SH sampleSpec = SampleGI_SH(scoord, PK_GI_SPEC_LVL);
        SceneGIMeta sampleMeta = SampleGI_Meta(scoord);

        const float sampleD = SampleLinearDepth(scoord);
        const float4 sampleNR = SampleWorldNormalRoughness(scoord);
        const float sampleDot = max(0.0f, dot(NR.xyz, sampleNR.xyz));
        const float sampleL = sqrt(SHToAmbientLuminance(sampleDiff));

        const float reproWeight = sampleMeta.history == 0u ? 0.125f : 1.0f;
        const float depthWeight = exp(-abs(D - sampleD) / max(D, sampleD));
        const float normalWeight = pow(sampleDot, 256.0);
        const float roughnessWeight = pow4(1.0f - abs(NR.w - sampleNR.w));
        const float sampleWeight = depthWeight * normalWeight * reproWeight;

        L = min(L, lerp(L, sampleL, sampleWeight));
        moments += float2(sampleL, sampleL * sampleL) * sampleWeight;
        W.x += sampleWeight;
        W.y += sampleWeight * roughnessWeight;

        //diffSH = AddSH(diffSH, sampleDiff, sampleWeight);
        //specSH = AddSH(specSH, sampleSpec, sampleWeight * roughnessWeight);
    }
    
    moments /= W.x;

    const float diffDist = min(1.0f, imageLoad(pk_ScreenGI_Hits, coord).r / 3.0f);
    meta.mindist = lerp(meta.mindist, diffDist, diffDist < meta.mindist ? 0.25f : 0.025f);
    
    meta.variance = moments.y - moments.x * moments.x;
    meta.variance = meta.variance / (L * L);
    meta.variance /= lerp(0.25f, 1.0f, meta.history01);

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(diffSH, 1.0f));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(specSH, 1.0f));
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

    SceneGIMeta meta = SampleGI_Meta(coord);

    if (meta.isOOB)
    {
        return;
    }

    float2 W = 1.0f.xx;

#if defined(PASS_VARIANCE)
    W = meta.history == 0u ? 0.125f.xx : 1.0f.xx;
    const float diffDist = min(1.0f, imageLoad(pk_ScreenGI_Hits, coord).r / 3.0f);
    meta.mindist = lerp(meta.mindist, diffDist, diffDist < meta.mindist ? 0.25f : 0.025f);
#endif

    SH diffSH = ScaleSH(SampleGI_SH(coord, PK_GI_DIFF_LVL), W.x);
    SH specSH = ScaleSH(SampleGI_SH(coord, PK_GI_SPEC_LVL), W.y);

    const float2 D = float2(SampleLinearDepth(coord), 1.0f - (SampleLinearDepth(coord) / pk_ProjectionParams.z));
    const float3 O = SampleViewPosition(coord, size, D.x);
    const float4 NR = SampleViewNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3x3 TBN = ComposeTBN(N);
    const float2 rotation = make_rotation(pk_FrameIndex * (PK_PI / 3.0f));

    const float normalPower = 0.25f + meta.history * 24.0f * D.y;
    const float pixelScale = pk_ScreenParams.w * 2.0f / pk_MATRIX_P[1][1];
    const float hitdistScale = lerp(2.0f, 100.0f, meta.mindist);
    const float depthScale = (pk_ProjectionParams.x + D.x);

#if defined(PASS_VARIANCE)
    const float varianceScale = meta.history == 0u ? 3.0f : 1.0f;
#else
    const float varianceScale = meta.variance * 4.0f + 0.5f;
#endif

    const float radius = pixelScale * hitdistScale * depthScale;// *varianceScale;
    const float planedist = radius * 0.5f;

    float L = sqrt(SHToAmbientLuminance(diffSH));
    float2 moments = float2(L, L * L) * W.x;

    float3 clipuvw;

    for (uint i = 0u; i < 16u; ++i)
    {
        float2 offset = PK_POISSON_DISK_POW2_16[i] * radius;
        offset = float2(offset.x * rotation.x - offset.y * rotation.y, offset.x * rotation.y + offset.y * rotation.x);

        const float3 samplePos = O + TBN * float3(offset.xy, 0.0f);

        if (!TryGetViewToClipUVW(samplePos, clipuvw))
        {
            continue;
        }

        const int2 scoord = int2(clipuvw.xy * size);
        const float4 sampleNR = SampleViewNormalRoughness(scoord);
        const float3 sampleV = SampleViewPosition(scoord, size) - O;
        const float sampleDistY = abs(dot(sampleV, N));
        const float sampleDist = dot(sampleV, sampleV);
        const float sampleDot = dot(N, sampleNR.xyz);

        if (sampleDistY > planedist || sampleDist > (radius * radius) || sampleDot <= 0.0)
        {
            continue;
        }

#if defined(PASS_VARIANCE)
        SceneGIMeta sampleMeta = SampleGI_Meta(scoord);
        const float reproWeight = sampleMeta.history == 0u ? 0.125f : 1.0f;
#else
        const float reproWeight = 1.0f;
#endif

        const float normalWeight = pow(sampleDot, normalPower);
        const float planeWeight = max(0.0f, 1.0f - (sampleDistY / planedist));
        const float distWeight = max(0.0f, 1.0f - (sqrt(sampleDist) / radius));
        const float roughnessWeight = pow4(1.0f - abs(sampleNR.w - NR.w));

        float2 WS;
        WS.x = saturate(normalWeight * planeWeight * distWeight * reproWeight);
        WS.y = WS.x * roughnessWeight;

        SH sampleDiff = SampleGI_SH(scoord, PK_GI_DIFF_LVL);
        SH sampleSpec = SampleGI_SH(scoord, PK_GI_SPEC_LVL);
        sampleDiff = DegenerateSH(sampleDiff, sampleDot);
        sampleSpec = DegenerateSH(sampleSpec, sampleDot);
        diffSH = AddSH(diffSH, sampleDiff, WS.x);
        specSH = AddSH(specSH, sampleSpec, WS.y);
        W += WS;
        
        const float sampleL = sqrt(SHToAmbientLuminance(sampleDiff));
        L = min(L, lerp(L, sampleL, WS.x));
    }

#if defined(PASS_VARIANCE)
    moments /= W.x;
    meta.variance = moments.y - moments.x * moments.x;
    meta.variance = meta.variance / (L * L);
    meta.variance /= lerp(0.25f, 1.0f, meta.history01);
    StoreGI_Meta(coord, meta);
#endif

    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(diffSH, 1.0f / W.x));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(specSH, 1.0f / W.y));
}