
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_program SHADER_STAGE_RAY_GENERATION MainRgs
#pragma pk_program SHADER_STAGE_RAY_MISS MainRms
#pragma pk_program SHADER_STAGE_RAY_CLOSEST_HIT MainRchs
#define PK_USE_SINGLE_DESCRIPTOR_SET

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
    const int2 raycoord = int2(gl_LaunchIDEXT.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(gl_LaunchIDEXT.xy, 1u);
    const float depth = PK_GI_SAMPLE_PREV_DEPTH(coord);

    if (Test_DepthFar(depth))
    {
        const float3 normal = SamplePreviousWorldNormal(coord);
        const float3 viewpos = GI_GetRayViewOrigin(coord, depth);
        const float3 viewdir = normalize(viewpos) * float3x3(pk_ViewToWorldPrev);
        const float3 normalOffset = GI_GetRayOriginNormalOffset(normal, viewdir);
        const float3 origin = ViewToWorldPosPrev(viewpos) + normalOffset;

        const Reservoir reservoir = ReSTIR_Load_Previous(raycoord);
        const float4 direction = normalizeLength(reservoir.position - origin);

        const float maxErrorDist = RESTIR_VALIDATION_ERROR_DIST * direction.w;
        const float maxErrorLuma = RESTIR_VALIDATION_ERROR_LUMA;
        const float tmax = direction.w + maxErrorDist;

        payload.HIT_LOGLUMINANCE = 0.0f;
        payload.HIT_DISTANCE = direction.w;
        traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, direction.xyz, tmax, 0);

        const float logLuminance = log(1.0f + dot(PK_LUMA_BT709, reservoir.radiance));
        const bool invalidDist = abs(direction.w - payload.HIT_DISTANCE) > maxErrorDist;
        const bool invalidLuma = abs(logLuminance - payload.HIT_LOGLUMINANCE) > maxErrorLuma;

        if (invalidDist || invalidLuma)
        {
            ReSTIR_StoreZero(raycoord);
        }
    }
}

void MainRms()
{
    const float3 worldpos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * payload.HIT_DISTANCE;
    float3 radiance = 0.0f.xxx;

    if (GI_IsScreenHit(worldpos, true))
    {
        const float2 uv = WorldToClipUVPrev(worldpos);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        radiance = SampleEnvironment(OctaUV(gl_WorldRayDirectionEXT), 0.0f);
    }

    payload.HIT_LOGLUMINANCE = log(1.0f + dot(PK_LUMA_BT709, radiance));
}

void MainRchs()
{
    const float3 worldpos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    float3 radiance = 0.0f.xxx;

    if (GI_IsScreenHit(worldpos, false))
    {
        const float2 uv = WorldToClipUVPrev(worldpos);
        radiance = SamplePreviousColor(uv);
    }
    else
    {
        const float4 voxel = GI_Load_Voxel(worldpos, PK_GI_GET_VX_MI_BIAS(gl_HitTEXT));
        radiance = voxel.rgb / max(voxel.a, 1e-2f);
    }

    payload.HIT_DISTANCE = gl_HitTEXT;
    payload.HIT_LOGLUMINANCE = log(1.0f + dot(PK_LUMA_BT709, radiance));
}
