#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SharedSceneGI.glsl
#include includes/CTASwizzling.glsl

float3 SampleRadiance(const int2 coord, const float3 origin, const float3 direction, const float dist, bool isMiss)
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
            float sdepth = SamplePreviousViewDepth(clipuvw.xy);
            isScreenHit = isMiss && !Test_DepthFar(sdepth);

            if (!isScreenHit)
            {
                float rdepth = ViewDepth(clipuvw.z);
                float sviewz = -SamplePreviousViewNormal(clipuvw.xy).z + 0.15f;
                isScreenHit = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
            }

            if (isScreenHit)
            {
                return SamplePreviousColor(clipuvw.xy);
            }
        }
    }

    if (isMiss)
    {
        return SampleEnvironment(OctaUV(direction), 0.0f);
    }

    const float4 voxel = GI_Load_Voxel(worldpos, PK_GI_RAY_CONE_SIZE * log2(1.0f + (dist / pk_GI_VoxelSize)));
    return voxel.rgb / max(voxel.a, 1e-2f);
}

uint2 GetSwizzledThreadID()
{
    return ThreadGroupTilingX
    (
        gl_NumWorkGroups.xy,
        uint2(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_8),
        8u,
        gl_LocalInvocationID.xy,
        gl_WorkGroupID.xy
    );
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(GetSwizzledThreadID());
    const float depth = SampleViewDepth(coord);

    if (!Test_DepthFar(depth))
    {
        GI_Store_Packed_SampleDiff(coord, uint4(0));
        GI_Store_Packed_SampleSpec(coord, uint2(0));
        return;
    }

    const float4 NR = SampleWorldNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3 O = SampleWorldPosition(coord, size, depth);
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);

    GIRayDirections directions = GI_GetRayDirections(coord, N, V, NR.w);
    GIRayHits hits = GI_Load_RayHits(coord);

    // Construct new samples
    GISampleDiff s_diff;
    s_diff.sh = SH_FromRadiance(SampleRadiance(coord, O, directions.diff, hits.distDiff, hits.isMissDiff), directions.diff);
    s_diff.ao = hits.isMissDiff ? 1.0f : saturate(hits.distDiff / PK_GI_RAY_MAX_DISTANCE);

    GISampleSpec s_spec;
    s_spec.radiance = SampleRadiance(coord, O, directions.spec, hits.distSpec, hits.isMissSpec);
    s_spec.ao = hits.isMissSpec ? 1.0f : saturate(hits.distSpec / PK_GI_RAY_MAX_DISTANCE);

    GI_Store_SampleDiff(coord, s_diff);
    GI_Store_SampleSpec(coord, s_spec);
}