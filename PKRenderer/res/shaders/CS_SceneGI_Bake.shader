#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

float3 SampleEnvironment(float3 direction, float roughness)
{
    return HDRDecode(tex2DLod(pk_SceneOEM_HDR, OctaUV(direction), 4.0f * roughness)).rgb * pk_SceneOEM_Exposure;
}

float3 SampleRadiance(const float3 origin, const float3 direction, const float dist, const float roughness)
{
    const float3 worldpos = origin + direction * dist;
    float3 clipuvw;

    // Try sample previous forward output for better sampling.
    if (TryGetWorldToPrevClipUVW(worldpos, clipuvw))
    {
        float rdepth = LinearizeDepth(clipuvw.z);
        float sdepth = SamplePreviousLinearDepth(clipuvw.xy);
        float sviewz = -SamplePreviousViewNormal(clipuvw.xy).z + 0.1f;
        float deltaDepth = abs(sdepth - rdepth);
        float bias = rdepth * 0.01f / sviewz;

        if ((sdepth > pk_ProjectionParams.z - 1e-4f && dist >= PK_GI_RAY_MAX_DISTANCE - 0.01f) || deltaDepth <= bias)
        {
            return tex2D(pk_ScreenColorPrevious, clipuvw.xy).rgb;
        }
    }

    const float level = roughness * roughness * log2(max(1.0f, dist) / PK_GI_VOXEL_SIZE);
    const float4 voxel = SampleGI_WS(worldpos, level);

    const float3 env = SampleEnvironment(direction, roughness);
    const float envclip = saturate(PK_GI_RAY_MAX_DISTANCE * (1.0f - (dist / PK_GI_RAY_MAX_DISTANCE)));
    const float alpha = max(voxel.a, 1.0f / (PK_GI_VOXEL_MAX_MIP * PK_GI_VOXEL_MAX_MIP));

    return lerp(env, voxel.rgb / alpha, envclip);
}

SH SampleIrradianceSH(const float3 O, const float3 N, int2 coord)
{
    const float2 Xi = GetSampleOffset(GlobalNoiseBlue(coord + pk_FrameIndex / PK_GI_SAMPLE_COUNT).xy);
    const float3 direction = ImportanceSampleGGX(Xi, N, 1.0f);
    const float sampleDistance = imageLoad(pk_ScreenGI_Hits, coord).r;
    const float3 radiance = SampleRadiance(O, direction, sampleDistance, 0.5f);
    return IrradianceToSH(radiance, direction);
}

SH SampleRadianceSH(const float3 O, const float3 N, const float3 V, int2 coord, float roughness)
{
    const float2 Xi = GetSampleOffset(GlobalNoiseBlue(coord + pk_FrameIndex / PK_GI_SAMPLE_COUNT).xy);
    const float3 direction = ImportanceSampleGGX(Xi, N, V, roughness);
    const float sampleDistance = imageLoad(pk_ScreenGI_Hits, coord).g;
    const float3 radiance = SampleRadiance(O, direction, sampleDistance, 0.0f);
    return IrradianceToSH(radiance, direction);
}

void AccumualteAO(const int2 coord, bool discontinuous)
{
    float pre = imageLoad(pk_ScreenGI_AO, coord).r;
    float cur = min(1.0f, imageLoad(pk_ScreenGI_Hits, coord).r);
    cur = discontinuous ? 1.0f : lerp(pre, cur, cur < pre ? 1.0f : 0.01f);
    imageStore(pk_ScreenGI_AO, coord, float4(cur));
}

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 size = imageSize(pk_ScreenGI_Mask).xy;
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

    // Find a base for the side cones with the normal as one of its base vectors.
    const float4 NR = SampleWorldNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3 O = SampleWorldPosition(coord, size);
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
    const float3 D = GlobalNoiseBlue(uint2(coord)).xyz;
    const float2 UV = (coord + 0.5f.xx) / size;

    SH irradianceSH = SampleGI_SH(UV, PK_GI_DIFF_LVL);
    SH radianceSH = SampleGI_SH(UV, PK_GI_SPEC_LVL);

    float irradianceLum = SHToLuminance(irradianceSH, N);
    float radianceLum = SHToLuminance(radianceSH, reflect(V, N));

    const bool refreshDiff = mask.discontinuityFrames > 7u || IsNaN(irradianceSH.SHY.w) || IsNaN(irradianceSH.CoCg);
    const bool refreshSpec = mask.discontinuityFrames > 7u || IsNaN(radianceSH.SHY.w) || IsNaN(radianceSH.CoCg);
    float interDiff = lerp(0.01f, 0.75f, saturate(irradianceLum * 5.0f));
    float interSpec = lerp(0.05f, 0.25f, saturate(radianceLum * 5.0f));

    //if (UV.x > 0.5f)
    //{
    //    interDiff = 0.005f;
    //    interSpec = 0.005f;
    //}

    SH irradianceNew = SampleIrradianceSH(O, N, coord);
    SH radianceNew = SampleRadianceSH(O, N, V, coord, NR.w);

    irradianceSH = refreshDiff ? irradianceNew : InterpolateSH(irradianceSH, irradianceNew, interDiff);
    radianceSH = refreshSpec ? radianceNew : InterpolateSH(radianceSH, radianceNew, interSpec);

    StoreGI_SH(coord, PK_GI_DIFF_LVL, irradianceSH);
    StoreGI_SH(coord, PK_GI_SPEC_LVL, radianceSH);

    AccumualteAO(coord, mask.isOOB || mask.discontinuityFrames > 7u);
}