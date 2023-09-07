#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedReSTIR.glsl

float3 SampleRadiance(const float3 origin, const float3 direction, const GIRayHit hit)
{
    const float3 worldpos = origin + direction * hit.dist;

    if (hit.isScreen)
    {
        float2 uv = ClipToUV(mul(pk_MATRIX_L_VP, float4(worldpos, 1.0f)).xyw);
        return SamplePreviousColor(uv);
    }

    if (hit.isMiss)
    {
        return SampleEnvironment(OctaUV(direction), 0.0f);
    }

    const float4 voxel = GI_Load_Voxel(worldpos, PK_GI_RAY_CONE_SIZE * log2(1.0f + (hit.dist / pk_GI_VoxelSize)));
    return voxel.rgb / max(voxel.a, 1e-2f);
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 raycoord = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(raycoord));
    const float depth = SampleViewDepth(coord);
    
    GIRayParams params;
    uint4 packedDiff = uint4(0u);
    uint2 packedSpec = uint2(0u);

    if (Test_DepthFar(depth))
    {
        GI_GET_RAY_PARAMS(coord, raycoord, depth, params)
    
        const GIRayHits hits = GI_Load_RayHits(raycoord);

        // Always use reservoir packing for diff hits.
        // They can be used for neighbour reconstruction outside of ReSTIR
        packedDiff = ReSTIR_Pack_Hit
        (
            params.diffdir,
            hits.diff.isMiss ? PK_GI_RAY_MAX_DISTANCE : hits.diff.dist,
            params.normal,
            hits.diffNormal,
            SampleRadiance(params.origin, params.diffdir, hits.diff)
        );

        #if PK_GI_APPROX_ROUGH_SPEC == 1
        [[branch]]
        if (params.roughness < PK_GI_MAX_ROUGH_SPEC)
        #endif
        {
            GISpec spec = pk_Zero_GISpec;
            spec.history = PK_GI_MAX_HISTORY;
            spec.radiance = SampleRadiance(params.origin, params.specdir, hits.spec);
            spec.ao = hits.spec.isMiss ? 1.0f : saturate(hits.spec.dist / PK_GI_RAY_MAX_DISTANCE);
            packedSpec = GI_Pack_Spec(spec);
        }
    }

    // Its slightly faster to use reservoirs texture than gi data texture
    ReSTIR_Store_Hit(raycoord, packedDiff);
    GI_Store_Packed_Spec(raycoord, packedSpec);
}