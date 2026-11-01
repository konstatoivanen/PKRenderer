
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_program RayGeneration MainRgs
#pragma pk_program RayMiss MainRms
#pragma pk_program RayClosestHit MainRchs

#include "includes/GBuffers.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/SceneGIVX.glsl"
#include "includes/SceneGIRT.glsl"
#include "includes/SceneGIReSTIR.glsl"

struct ValidatePayload
{
    float hit_logluminance;
    float hit_distance;
};

void MainRgs()
{
    // Only validate half of the reservervoirs each frame. saves performance.
    const int2 coord_ray = int2(gl_LaunchIDEXT.xy) * int2(1, 2) + int2(0, (pk_View.FrameIndex.y / 2u) & 0x1u);
    const int2 coord = GI_ExpandCheckerboardCoord(coord_ray, 1u);
    const float depth = PK_GI_SAMPLE_PREV_DEPTH(coord);

    bool invalid_dist = false;
    bool invalid_luma = false;

    if (Test_DepthIsScene(depth))
    {
        const float3 normal = SamplePreviousWorldNormal(coord);
        const float3 view_pos = GI_GetRayViewOrigin(coord, depth);
        const float3 view_dir = normalize(view_pos) * float3x3(pk_View.ViewToWorldPrev);
        const float3 normal_offset = GI_GetRayOriginNormalOffset(normal, view_dir);
        const float3 origin = ViewToWorldPosPrev(view_pos) + normal_offset;

        const Reservoir reservoir = ReSTIR_Load_Previous(coord_ray);
        const float4 direction = NormalizeLength(reservoir.position - origin);

        const float max_error_dist = RESTIR_VALIDATION_ERROR_DIST * direction.w;
        const float max_error_luma = RESTIR_VALIDATION_ERROR_LUMA;
        const float t_max = direction.w + max_error_dist;
        
        RayDesc ray;
        ray.Origin = origin;
        ray.Direction = direction.xyz;
        ray.TMin = 0.0f;
        ray.TMax = t_max;

        ValidatePayload payload = ValidatePayload(0.0f, direction.w);
        TraceRay(pk_SceneStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, ray, payload);

        const float log_luma = log(1.0f + dot(PK_LUMA_BT709, reservoir.radiance));
        invalid_dist = abs(direction.w - payload.hit_distance) > max_error_dist;
        invalid_luma = abs(log_luma - payload.hit_logluminance) > max_error_luma;
    }

    if (invalid_dist || invalid_luma || pk_View.FrameIndex.y < 1u)
    {
        ReSTIR_StoreZero(coord_ray);
    }
}

void MainRms(inout ValidatePayload payload)
{
    const float3 world_pos = WorldRayOrigin() + WorldRayDirection() * payload.hit_distance;
    float3 radiance = 0.0f.xxx;

    if (GI_IsScreenHit(world_pos, true))
    {
        const float2 uv = WorldToClipUvPrev(world_pos);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        radiance = SceneEnv_Sample_IBL(EncodeOctaUv(WorldRayDirection()), 0.0f);
    }

    payload.hit_logluminance = log(1.0f + dot(PK_LUMA_BT709, radiance));
}

void MainRchs(inout ValidatePayload payload, in BuiltInTriangleIntersectionAttributes attribs)
{
    const float3 world_pos = WorldRayOrigin() + WorldRayDirection() * RayTCurrent();
    float3 radiance = 0.0f.xxx;

    if (GI_IsScreenHit(world_pos, false))
    {
        const float2 uv = WorldToClipUvPrev(world_pos);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        const float4 voxel = GI_VX_Load(world_pos, PK_GI_GET_VX_MI_BIAS(RayTCurrent()));
        radiance = voxel.rgb / max(voxel.a, 1e-2f);
    }

    payload.hit_distance = RayTCurrent();
    payload.hit_logluminance = log(1.0f + dot(PK_LUMA_BT709, radiance));
}