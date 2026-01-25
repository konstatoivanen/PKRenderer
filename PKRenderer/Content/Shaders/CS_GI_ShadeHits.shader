
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_program SHADER_STAGE_COMPUTE main

#define PK_USE_SINGLE_DESCRIPTOR_SET
#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 1

#include "includes/GBuffers.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/SceneGIVX.glsl"
#include "includes/SceneGIRT.glsl"
#include "includes/SceneGIReSTIR.glsl"

float3 SampleRadiance(const float3 origin, const float3 direction, const GIRayHit hit)
{
    const float3 world_pos = origin + direction * hit.dist;

    if (hit.is_screen)
    {
        float2 uv = WorldToClipUvPrev(world_pos);
        return SamplePreviousColor(uv);
    }

    if (hit.is_miss)
    {
        return SceneEnv_Sample_IBL(EncodeOctaUv(direction), 0.0f);
    }

    const float4 voxel = GI_Load_Voxel(world_pos, PK_GI_GET_VX_MI_BIAS(hit.dist));
    return voxel.rgb / max(voxel.a, 1e-2f);
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 coord_ray = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(coord_ray));
    const float depth = PK_GI_SAMPLE_DEPTH(coord);

    uint4 packed_diff = uint4(0u);
    uint2 packed_spec = uint2(0u);

    [[branch]]
    if (Test_DepthIsScene(depth))
    {
        const float4 normal_roughness = SampleWorldNormalRoughness(coord);
        const GIRayHits hits = GI_Load_RayHits(coord_ray);

        GI_LOAD_RAY_PARAMS(coord, coord_ray, depth, normal_roughness.xyz, normal_roughness.w)

        // Convert ray to unbiased space
        const float3 hitpos = origin + direction_diff * lerp(hits.diff.dist, PK_GI_RAY_TMAX, hits.diff.is_miss);
        const float3 origin_unbiased = CoordToWorldPos(coord, depth);
        const float4 hitvec_unbiased = NormalizeLength(hitpos - origin_unbiased);
        // assuming lambertian distribution
        const float inverse_pdf = PK_PI * SafePositiveRcp(dot(normal_roughness.xyz, hitvec_unbiased.xyz));

        // Always use reservoir packing for diff hits.
        // They can be used for neighbour reconstruction outside of ReSTIR
        packed_diff = ReSTIR_Pack_Hit
        (
            hitvec_unbiased.xyz,
            hitvec_unbiased.w,
            inverse_pdf,
            hits.diff_normal,
            SampleRadiance(origin, direction_diff, hits.diff)
        );

#if PK_GI_APPROX_ROUGH_SPEC == 1
        [[branch]]
        if (normal_roughness.w < PK_GI_MAX_ROUGH_SPEC)
#endif
        {
            GISpec spec = PK_GI_SPEC_ZERO;
            spec.radiance = SampleRadiance(origin, direction_spec, hits.spec);
            spec.ao = hits.spec.is_miss ? 1.0f : saturate(hits.spec.dist / PK_GI_RAY_TMAX);
            spec.history = PK_GI_SPEC_MAX_HISTORY;
            packed_spec = GI_Pack_Spec(spec);
        }
    }

    GI_Store_Packed_Diff(coord_ray, packed_diff);
    GI_Store_Packed_Spec(coord_ray, packed_spec);
}