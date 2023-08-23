#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_RESTIR

#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedRestir.glsl
#include includes/CTASwizzling.glsl

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
    const int2 raycoord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_8, 8u));
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(raycoord));
    const float depth = SampleViewDepth(coord);
    
    GIRayParams params;
    uint4 packedDiff = uint4(0u);
    uint2 packedSpec = uint2(0u);

    if (Test_DepthFar(depth))
    {
        GI_GET_RAY_PARAMS(coord, raycoord, depth, params)
    
        GIDiff diff = pk_Zero_GIDiff;
        GISpec spec = pk_Zero_GISpec;
        const GIRayHits hits = GI_Load_RayHits(raycoord);
        
        const float3 radianceDiff = SampleRadiance(params.origin, params.diffdir, hits.diff);
        diff.history = PK_GI_MAX_HISTORY;
        diff.sh = SH_FromRadiance(radianceDiff, params.diffdir);
        diff.ao = hits.diff.isMiss ? 1.0f : saturate(hits.diff.dist / PK_GI_RAY_MAX_DISTANCE);
    
        #if PK_GI_APPROX_ROUGH_SPEC == 1
        [[branch]]
        if (params.roughness < PK_GI_MAX_ROUGH_SPEC)
        #endif
        {
            spec.history = PK_GI_MAX_HISTORY;
            spec.radiance = SampleRadiance(params.origin, params.specdir, hits.spec);
            spec.ao = hits.spec.isMiss ? 1.0f : saturate(hits.spec.dist / PK_GI_RAY_MAX_DISTANCE);
        }

        packedDiff = GI_Pack_Diff(diff);
        packedSpec = GI_Pack_Spec(spec);
        
        #if defined(PK_GI_RESTIR)
        // @TODO replace this with proper geometry normal
        const float inversePdf = PK_PI * safePositiveRcp(dot(params.diffdir, SampleWorldNormal(coord)));
        const float3 hitnor = DecodeOctaUV(imageLoad(pk_GI_RayHitNormals, raycoord).r);
        const float3 hitpos = params.origin + params.diffdir * (hits.diff.isMiss ? PK_GI_RAY_MAX_DISTANCE : hits.diff.dist);
        Restir_Store_Hit(raycoord, hitpos, hitnor, radianceDiff, inversePdf);
        Restir_CopyToPrev(raycoord);
        #endif
    }

    GI_Store_Packed_Diff(coord, packedDiff);
    GI_Store_Packed_Spec(coord, packedSpec);

    #if defined(PK_GI_CHECKERBOARD_TRACE)
    // Fill blanks in neighbourhood to avoid nans on resize
    const int2 ncoord = int2(raycoord.x * 2 + GI_GetCheckerboardOffset(uint2(raycoord), pk_FrameIndex.y + 1u), raycoord.y);
    GI_Store_Packed_Diff(ncoord, packedDiff);
    GI_Store_Packed_Spec(ncoord, packedSpec);
    #endif
}