#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 1

#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SceneGIVX.glsl
#include includes/SceneGIRT.glsl
#include includes/SceneGIReSTIR.glsl

float3 SampleRadiance(const float3 origin, const float3 direction, const GIRayHit hit)
{
    const float3 worldpos = origin + direction * hit.dist;

    if (hit.isScreen)
    {
        float2 uv = WorldToClipUVPrev(worldpos);
        return SamplePreviousColor(uv);
    }

    if (hit.isMiss)
    {
        return SampleEnvironment(OctaUV(direction), 0.0f);
    }

    const float4 voxel = GI_Load_Voxel(worldpos, PK_GI_VX_CONE_SIZE * log2(1.0f + (hit.dist / pk_GI_VoxelSize)));
    return voxel.rgb / max(voxel.a, 1e-2f);
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 raycoord = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(raycoord));
    const float depth = PK_GI_SAMPLE_DEPTH(coord);

    uint4 packedDiff = uint4(0u);
    uint2 packedSpec = uint2(0u);

    [[branch]]
    if (Test_DepthFar(depth))
    {
        const float4 normalRoughness = SampleWorldNormalRoughness(coord);
        const GIRayHits hits = GI_Load_RayHits(raycoord);

        GI_LOAD_RAY_PARAMS(coord, raycoord, depth, normalRoughness.xyz, normalRoughness.w)

        // Convert ray to unbiased space
        const float3 hitpos = origin + directionDiff * lerp(hits.diff.dist, PK_GI_RAY_TMAX, hits.diff.isMiss);
        const float3 unbiasedOrigin = CoordToWorldPos(coord, depth);
        const float4 unbiasedHitVec = normalizeLength(hitpos - unbiasedOrigin);

        // Always use reservoir packing for diff hits.
        // They can be used for neighbour reconstruction outside of ReSTIR
        packedDiff = ReSTIR_Pack_Hit
        (
            unbiasedHitVec.xyz,
            unbiasedHitVec.w,
            normalRoughness.xyz,
            hits.diffNormal,
            SampleRadiance(origin, directionDiff, hits.diff)
        );

#if PK_GI_APPROX_ROUGH_SPEC == 1
        [[branch]]
        if (normalRoughness.w < PK_GI_MAX_ROUGH_SPEC)
#endif
        {
            GISpec spec = PK_GI_SPEC_ZERO;
            spec.radiance = SampleRadiance(origin, directionSpec, hits.spec);
            spec.ao = hits.spec.isMiss ? 1.0f : saturate(hits.spec.dist / PK_GI_RAY_TMAX);
            spec.history = PK_GI_SPEC_MAX_HISTORY;
            packedSpec = GI_Pack_Spec(spec);
        }
    }

    GI_Store_Packed_Diff(raycoord, packedDiff);
    GI_Store_Packed_Spec(raycoord, packedSpec);
}