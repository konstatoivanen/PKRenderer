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
    const float4 voxel = GI_Load_Voxel(worldpos, level);
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

    GIRayDirections directions = GI_GetRayDirections(coord, N, V, NR.w);
    GIRayHits hits = GI_Load_RayHits(coord);
    GISampleFull filtered = GI_Load_SampleFull(coord);
    const float wDiff = max(1.0f / (filtered.meta.historyDiff + 1.0f), 0.01f);
    const float wSpec = max(1.0f / (filtered.meta.historySpec + 1.0f), 0.01f); 
    
    const float coneSizeDiff = 0.5f;
    const float coneSizeSpec = pow2(NR.w);
    
    float3 radianceDiff = SampleRadiance(coord, O, directions.diff, hits.distDiff, hits.isMissDiff, coneSizeDiff);
    float3 radianceSpec = SampleRadiance(coord, O, directions.spec, hits.distSpec, hits.isMissSpec, coneSizeSpec);

    // Construct new samples
    GISampleDiff s_diff;
    GISampleSpec s_spec;
    s_diff.sh       = SHFromRadiance(radianceDiff, directions.diff);
    s_spec.radiance = radianceSpec;
    s_diff.ao       = hits.isMissDiff ? 1.0f : saturate(hits.distDiff / PK_GI_AO_DIFF_MAX_DISTANCE);
    s_spec.ao       = hits.isMissSpec ? 1.0f : saturate(hits.distSpec / PK_GI_AO_SPEC_MAX_DISTANCE);
    float luma      = SHToLuminance(s_diff.sh, N) + dot(pk_Luminance.rgb, radianceSpec);

    // Interpolate samples
    filtered.diff.sh       = SHInterpolate(filtered.diff.sh, s_diff.sh, wDiff);
    filtered.spec.radiance = lerp(filtered.spec.radiance, s_spec.radiance, wSpec);
    filtered.diff.ao       = lerp(filtered.diff.ao, s_diff.ao, wDiff);
    filtered.spec.ao       = lerp(filtered.spec.ao, s_spec.ao, wSpec);
    filtered.meta.moments  = lerp(filtered.meta.moments, float2(luma, pow2(luma)), wDiff);
    filtered.spec.depth    = depth;
    filtered.diff.depth    = depth;

    GI_Store_SampleFull(coord, filtered);
}