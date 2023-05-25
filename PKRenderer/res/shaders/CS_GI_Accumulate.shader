#version 460
#pragma PROGRAM_COMPUTE
#include includes/Lighting.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

float3 SampleRadiance(const int2 coord, const float3 origin, const float3 direction, const float dist, bool isMiss, const float roughness)
{
    const float3 worldpos = origin + direction * dist;
    float3 clipuvw;

    // Try sample previous forward output for better sampling.
    if (Test_WorldToPrevClipUVW(worldpos, clipuvw))
    {
        float2 deltacoord = abs(coord - (clipuvw.xy * pk_ScreenParams.xy));
        bool isScreenHit = dot(deltacoord, deltacoord) > 2.0f;

        if (isScreenHit)
        {
            float sdepth = SamplePreviousLinearDepth(clipuvw.xy);
            isScreenHit = isMiss && !Test_DepthFar(sdepth);

            if (!isScreenHit)
            {
                float rdepth = LinearizeDepth(clipuvw.z);
                float sviewz = -SamplePreviousViewNormal(clipuvw.xy).z + 0.15f;
                isScreenHit = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
            }

            if (isScreenHit)
            {
                return tex2D(pk_ScreenColorPrevious, clipuvw.xy).rgb;
            }
        }
    }

    if (isMiss)
    {
        return SampleEnvironment(OctaUV(direction), roughness);
    }

    const float level = roughness * roughness * log2(max(1.0f, dist) / pk_GI_VoxelSize);
    const float4 voxel = GI_Load_VX(worldpos, level);
    return voxel.rgb / max(voxel.a, 1.0f / (PK_GI_VOXEL_MAX_MIP * PK_GI_VOXEL_MAX_MIP));
}

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    const float depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    const float4 NR = SampleWorldNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3 O = SampleWorldPosition(coord, size, depth);
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);

    float3 dirDiff, dirSpec;
    GI_GetRayDirections(coord, pk_FrameIndex, N, V, NR.w, dirDiff, dirSpec);

    const uint packedHits = imageLoad(pk_GI_RayHits, coord).x;
    const bool isMissDiff = bitfieldExtract(packedHits, 0, 16) == 0xFFFF;
    const bool isMissSpec = bitfieldExtract(packedHits, 16, 16) == 0xFFFF;
    const float2 hitDist = unpackHalf2x16(packedHits);

    const float history = GI_Load_HistoryVariance(coord).x;
    const float wDiff = max(1.0f / min(history + 2.0f, PK_GI_MAX_HISTORY), 0.01f);
    const float wSpec = max(1.0f / min(history + 1.0f, PK_GI_MAX_HISTORY), 0.01f);//exp(-NR.w) * 0.1f);

    const float coneSizeDiff = 0.5f;
    const float coneSizeSpec = pow2(NR.w);

    float3 radianceDiff = SampleRadiance(coord, O, dirDiff, hitDist.x, isMissDiff, coneSizeDiff);
    float3 radianceSpec = SampleRadiance(coord, O, dirSpec, hitDist.y, isMissSpec, coneSizeSpec);

    // Fallback on ws cache on fresh samples
    if (history < 4)
    {
        float4 voxel = GI_Load_VX(O, 0.0f);
        voxel.rgb /= max(voxel.a, 1.0f / (PK_GI_VOXEL_MAX_MIP * PK_GI_VOXEL_MAX_MIP));
        voxel.rgb *= dot(N, dirDiff) / (history + 1);
        radianceDiff = max(radianceDiff, voxel.rgb);
    }

    const SH sampleDiff = SHFromRadiance(radianceDiff, dirDiff);
    const SH sampleSpec = SHFromRadiance(radianceSpec, dirSpec);

    SH diff = history < 1.0f ? sampleSpec : GI_Load_SH(coord, PK_GI_DIFF_LVL);
    SH spec = GI_Load_SH(coord, PK_GI_SPEC_LVL);

    float lumaA = GI_SHToLuminance(diff, N);
    float lumaB = GI_SHToLuminance(sampleDiff, N);

    float2 prevMoments = history < 1.0f ? float2(lumaA, pow2(lumaA)) : GI_Load_Moments(coord);
    float2 moments = float2(lumaB, pow2(lumaB));
    moments = lerp(prevMoments, moments, wDiff);

    diff = SHInterpolate(diff, sampleDiff, wDiff);
    spec = SHInterpolate(spec, sampleSpec, wSpec);

    GI_Store_Moments(coord, moments);
    GI_Store_HistoryVariance(coord, float2(history, 0.0f));
    GI_Store_SH(coord, PK_GI_DIFF_LVL, diff);
    GI_Store_SH(coord, PK_GI_SPEC_LVL, spec);
}