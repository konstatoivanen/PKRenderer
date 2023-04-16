#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

float3 SampleEnvironment(float3 direction, float roughness)
{
    return HDRDecode(tex2DLod(pk_SceneOEM_HDR, OctaUV(direction), 4.0f * roughness)).rgb * pk_SceneOEM_Exposure;
}

float3 SampleRadiance(const int2 coord, const float3 origin, const float3 direction, const float dist, const float roughness)
{
    const float3 worldpos = origin + direction * dist;
    float3 clipuvw;

    // Try sample previous forward output for better sampling.
    if (TryGetWorldToPrevClipUVW(worldpos, clipuvw))
    {
        float sdepth = SamplePreviousLinearDepth(clipuvw.xy);
        bool isMiss = sdepth > pk_ProjectionParams.z - 1e-4f && dist >= PK_GI_RAY_MAX_DISTANCE - 0.01f;
        
        float rdepth = LinearizeDepth(clipuvw.z);
        float sviewz = -SamplePreviousViewNormal(clipuvw.xy).z + 0.15f;
        bool isDepthValid = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);

        float2 deltacoord = abs(coord - (clipuvw.xy * pk_ScreenParams.xy));
        bool isCoordValid = dot(deltacoord, deltacoord) > 2.0f;

        if (isCoordValid && (isMiss || isDepthValid))
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

    const float4 NR = SampleWorldNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3 O = SampleWorldPosition(coord, size);
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
    const float2 Xi = GetSampleOffset(coord, pk_FrameIndex);
    const float2 hitDist = imageLoad(pk_ScreenGI_Hits, coord).xy;

    const float3 diffDir = ImportanceSampleLambert(Xi, N);
    const float3 specDir = ImportanceSampleSmithGGX(Xi, N, V, NR.w);

    const float3 diffRad = SampleRadiance(coord, O, diffDir, hitDist.x, 0.5f);
    const float3 specRad = SampleRadiance(coord, O, specDir, hitDist.y, 0.0f);

    SH diffSH = SampleGI_SH(coord, PK_GI_DIFF_LVL);
    SH specSH = SampleGI_SH(coord, PK_GI_SPEC_LVL);
    SH diffSHSample = IrradianceToSH(diffRad, diffDir);
    SH specSHSample = IrradianceToSH(specRad, specDir);

    float diffLum = SHToLuminance(diffSH, N);
    float specLum = SHToLuminance(specSH, reflect(V, N));

    const bool refreshDiff = meta.history == 0u || IsNaN(diffSH.SHY.w) || IsNaN(diffSH.CoCg);
    const bool refreshSpec = meta.history == 0u || IsNaN(specSH.SHY.w) || IsNaN(specSH.CoCg);
    float interDiff = lerp(0.01f, 0.5f, saturate(diffLum * 5.0f));
    float interSpec = lerp(0.05f, 0.5f, saturate(specLum * 5.0f));

    diffSH = refreshDiff ? diffSHSample : InterpolateSH(diffSH, diffSHSample, interDiff);
    specSH = refreshSpec ? specSHSample : InterpolateSH(specSH, specSHSample, interSpec);

    StoreGI_SH(coord, PK_GI_DIFF_LVL, diffSH);
    StoreGI_SH(coord, PK_GI_SPEC_LVL, specSH);
}