#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    int2 size = imageSize(pk_ScreenGI_SHY_Write).xy;
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    GIMask mask = LoadGIMask(coord);

    if (mask.isOOB)
    {
        return;
    }

    const float2 UV = (coord + 0.5f.xx) / size;
    const float D = SampleLinearDepth(UV);
    const float3 O = SampleViewPosition(coord, size, D);
    const float4 NR = SampleViewNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3x3 TBN = ComposeTBN(N);
    const float2 rotation = make_rotation(pk_SceneGI_SampleIndex * (PK_PI / 3.0f));

    const float mindist = imageLoad(pk_ScreenGI_AO, coord).r;
    const float radius = min(mindist, 0.02f * D) * (1.0f + mask.discontinuityFrames);
    const float planedist = radius * 0.5f;

    float3 clipuvw;

    float2 W = 1.0f.xx / (1.0f + mask.discontinuityFrames);
    SH irradiance = ScaleSH(SampleGI_SH(UV, PK_GI_DIFF_LVL), W.x);
    SH radiance = ScaleSH(SampleGI_SH(UV, PK_GI_SPEC_LVL), W.y);

    //if (UV.x > 0.5f)
    //{
    //	StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(irradiance, 1.0f / W.x));
    //	StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(radiance, 1.0f / W.y));
    //	return;
    //}

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
        const float3 sampleN = SampleViewNormal(scoord);
        const float3 sampleV = SampleViewPosition(scoord, size) - O;
        const float sampleDistY = abs(dot(sampleV, N));
        const float sampleDist = dot(sampleV, sampleV);
        const float sampleDot = dot(N, sampleN);

        if (sampleDistY > planedist || sampleDist > (radius * radius) || sampleDot < 0)
        {
            continue;
        }

        GIMask sampleMask = LoadGIMask(scoord);

        const float reproWeight = 1.0f / (1.0f + sampleMask.discontinuityFrames * 2);
        const float normalWeight = pow(max(0.0f, sampleDot), 9.0f - mask.discontinuityFrames);
        const float planeWeight = max(0.0f, 1.0f - (sampleDistY / planedist));
        const float distWeight = max(0.0f, 1.0f - (sqrt(sampleDist) / radius));

        float2 WS;
        WS.x = normalWeight * planeWeight * distWeight * reproWeight;
        WS.y = WS.x * pow2(NR.w);

        SH diffSH = SampleGI_SH(scoord, PK_GI_DIFF_LVL);
        SH specSH = SampleGI_SH(scoord, PK_GI_SPEC_LVL);

        diffSH = DegenerateSH(diffSH, sampleDot);
        specSH = DegenerateSH(specSH, sampleDot);

        irradiance = AddSH(irradiance, diffSH, WS.x);
        radiance = AddSH(radiance, specSH, WS.y);
        W += WS;
    }

    StoreGI_SH(coord, PK_GI_DIFF_LVL, ScaleSH(irradiance, 1.0f / W.x));
    StoreGI_SH(coord, PK_GI_SPEC_LVL, ScaleSH(radiance, 1.0f / W.y));
}