
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_program SHADER_STAGE_RAY_GENERATION MainRgs
#pragma pk_program SHADER_STAGE_RAY_MISS MainRms
#pragma pk_program SHADER_STAGE_RAY_CLOSEST_HIT MainRchs

#include "includes/GBuffers.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/SceneGIVX.glsl"
#include "includes/SceneGIRT.glsl"
#include "includes/SceneGIReSTIR.glsl"

#define HIT_LOGLUMINANCE x
#define HIT_DISTANCE y

PK_DECLARE_RT_PAYLOAD(float2, payload, 0);

void MainRgs()
{
    const int2 coord_ray = int2(gl_LaunchIDEXT.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(gl_LaunchIDEXT.xy, 1u);
    const float depth = PK_GI_SAMPLE_PREV_DEPTH(coord);

    bool invalid_dist = false;
    bool invalid_luma = false;

    if (Test_DepthIsScene(depth))
    {
        const float3 normal = SamplePreviousWorldNormal(coord);
        const float3 view_pos = GI_GetRayViewOrigin(coord, depth);
        const float3 view_dir = normalize(view_pos) * float3x3(pk_ViewToWorldPrev);
        const float3 normal_offset = GI_GetRayOriginNormalOffset(normal, view_dir);
        const float3 origin = ViewToWorldPosPrev(view_pos) + normal_offset;

        const Reservoir reservoir = ReSTIR_Load_Previous(coord_ray);
        const float4 direction = NormalizeLength(reservoir.position - origin);

        const float max_error_dist = RESTIR_VALIDATION_ERROR_DIST * direction.w;
        const float max_error_luma = RESTIR_VALIDATION_ERROR_LUMA;
        const float t_max = direction.w + max_error_dist;

        payload.HIT_LOGLUMINANCE = 0.0f;
        payload.HIT_DISTANCE = direction.w;
        traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, direction.xyz, t_max, 0);

        const float log_luma = log(1.0f + dot(PK_LUMA_BT709, reservoir.radiance));
        invalid_dist = abs(direction.w - payload.HIT_DISTANCE) > max_error_dist;
        invalid_luma = abs(log_luma - payload.HIT_LOGLUMINANCE) > max_error_luma;
    
    }

    if (invalid_dist || invalid_luma || pk_FrameIndex.y < 1u)
    {
        ReSTIR_StoreZero(coord_ray);
    }
}

void MainRms()
{
    const float3 world_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * payload.HIT_DISTANCE;
    float3 radiance = 0.0f.xxx;

    if (GI_IsScreenHit(world_pos, true))
    {
        const float2 uv = WorldToClipUvPrev(world_pos);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        radiance = SceneEnv_Sample_IBL(EncodeOctaUv(gl_WorldRayDirectionEXT), 0.0f);
    }

    payload.HIT_LOGLUMINANCE = log(1.0f + dot(PK_LUMA_BT709, radiance));
}

void MainRchs()
{
    const float3 world_pos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    float3 radiance = 0.0f.xxx;

    if (GI_IsScreenHit(world_pos, false))
    {
        const float2 uv = WorldToClipUvPrev(world_pos);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        const float4 voxel = GI_Load_Voxel(world_pos, PK_GI_GET_VX_MI_BIAS(gl_HitTEXT));
        radiance = voxel.rgb / max(voxel.a, 1e-2f);
    }

    payload.HIT_DISTANCE = gl_HitTEXT;
    payload.HIT_LOGLUMINANCE = log(1.0f + dot(PK_LUMA_BT709, radiance));
}
