#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl

struct TracePayload
{
    uint hitDistance;
};

#pragma PROGRAM_RAY_GENERATION

#define STEP_COUNT 32u
#define MAX_MIP 8s
#define HIT_TOLERANCE -0.01f
#define RT_MISS 0u
#define RT_HIT 1u
#define RT_SKY 2u

uint RayTraceScreen(const float2 uv, const float depth, const float3 ws_dir, inout float3 hit)
{
    const uint MIN_OCCUPANCY = gl_SubgroupSize - gl_SubgroupSize / 4;
    const float3 start = float3(uv, ProjectDepth(depth)) * 2.0f - 1.0f;
    const float4 end = float4(start.xyz, 1.0f) + WorldToClipDir(ws_dir);

    float3 cs_step = (end.xyz / end.w) - start;
    cs_step *= cmin((sign(cs_step.xy) - start.xy) / cs_step.xy);

    const half3 dir = half3(normalize(cs_step));
    const half3 inv = 1.0hf / abs(dir);
    const half2 tx = half2(pk_ScreenParams.zw);
    const half4 st = lerp(half4(1,1,0,0), half4(-1,-1,1,1), greaterThan(dir.xy, 0.0hf.xx).xyxy) * inv.xyxy;
    const byte isFwd = byte(dir.z > 0.0hf);

    short mip = 0s;
    half advance = 0.0hf;
    hit = start * 0.5f + 0.5f;

    for (uint i = 0u; i < STEP_COUNT && All_InArea(hit, 0.0f.xxx, 1.0f.xxx); ++i, --mip)
    {
        uint4 ballot = subgroupBallot(true);
        if (subgroupBallotBitCount(ballot) < MIN_OCCUPANCY)
        {
            break;
        }
    
        const half2 s_tx = tx * exp2(half(mip));
        const half2 s_px = half2(hit.xy) / s_tx;
        const half s_a = cmin(fma(st.xy, fract(s_px), st.zw) * s_tx);
        const half s_z = half(ProjectDepth(max(SampleMinZ(hit.xy, mip), SampleMinZ(int2(s_px), mip))) - hit.z) * inv.z;

        if (s_z > s_a * isFwd)
        {
            advance = s_a + 0.5hf * tx.y;
            hit = fma(float3(dir), float(advance).xxx, hit);
            mip = min(mip + 2s, MAX_MIP);
        }
        else if (mip <= 0)
        {
            bool isOccluded = s_z < HIT_TOLERANCE * hit.z;
            half refinement = max(s_z, cmax(-fma(st.xy, fract(half2(hit.xy) / tx), st.zw) * tx));
            hit += dir * lerp(refinement, -advance, isOccluded);
            // This has false negatives when hitting steep slopes
            // Causes hw rt to miss surfaces due to hit distance over estimation
            // @TODO FIX ME
            return isOccluded ? RT_MISS : RT_HIT;
        }
    }

    bool isSky = hit.z >= 1.0f;
    hit += dir * -advance;
    return isSky ? RT_SKY : RT_MISS;
}

uint SSRTRay(const float3 origin, const float3 direction, const float2 uv, const float depth, inout float hitDistance)
{
#if PK_GI_SSRT_PRETRACE == 1
    float3 hitpos = 0.0f.xxx;
    const uint result = RayTraceScreen(uv, depth, direction, hitpos);
    hitpos = SampleWorldPosition(hitpos.xy, LinearizeDepth(hitpos.z));
    hitDistance = max(0.0f, dot(normalize(direction), hitpos - origin));
    return result;
#else
    hitDistance = 0.0f;
    return RT_MISS;
#endif
}

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_LaunchIDEXT.xy);
    const float2 uv = (coord + 0.5f.xx) / size;
    float depth = SampleLinearDepth(coord);

    if (Test_DepthFar(depth))
    {
        // Apply bias to avoid rays clipping with geo at high distances
        depth -= depth * 1e-2f; 

        const float4 NR = SampleWorldNormalRoughness(coord);
        const float3 N = NR.xyz;

        const float3 O = SampleWorldPosition(coord, size, depth);
        const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);

        // @TODO diffuse ray directions for rough surfaces cause a lot of self intersections.
        // Investigate.
        GIRayDirections directions = GI_GetRayDirections(coord, N, V, NR.w);
        GIRayHits hits;
        
#if PK_GI_APPROX_ROUGH_SPEC == 1
        if (NR.w >= PK_GI_MAX_ROUGH_SPEC)
        {
            hits.distSpec = uintBitsToFloat(0xFFFFFFFFu);
            hits.isMissSpec = true;
        }
        else
#endif
        {
            const uint result = SSRTRay(O, directions.spec, uv, depth, hits.distSpec);
            hits.isMissSpec = result != RT_HIT;

            if (result == RT_MISS)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, hits.distSpec, directions.spec, PK_GI_RAY_MAX_DISTANCE, 0);
                hits.distSpec = uintBitsToFloat(payload.hitDistance);
                hits.isMissSpec = payload.hitDistance == 0xFFFFFFFFu;
            }
        }

        {
            const uint result = SSRTRay(O, directions.diff, uv, depth, hits.distDiff);
            hits.isMissDiff = result != RT_HIT;

            if (result == RT_MISS)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, hits.distDiff, directions.diff, PK_GI_RAY_MAX_DISTANCE, 0);
                hits.distDiff = uintBitsToFloat(payload.hitDistance);
                hits.isMissDiff = payload.hitDistance == 0xFFFFFFFFu;
            }
        }

        GI_Store_RayHits(coord, hits);
    }
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main() 
{
    payload.hitDistance = 0xFFFFFFFFu;
}

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
    payload.hitDistance = floatBitsToUint(PK_GET_RAY_HIT_DISTANCE);
}
