#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_SSRT_PRETRACE

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

uint SreenSpaceRaymarch(const float3 ws_origin, const float3 ws_dir, inout float3 hit)
{
    const uint MIN_OCCUPANCY = gl_SubgroupSize - gl_SubgroupSize / 4;
    const float4 start_clip = WorldToClipPos(ws_origin);
    const float3 start = start_clip.xyz / start_clip.w;
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
        const half s_z = half(ClipDepth(max(SampleMinZ(hit.xy, mip), SampleMinZ(int2(s_px), mip))) - hit.z) * inv.z;

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

bool IsScreenHit(const int2 coord, const float3 origin, const float3 direction, const GIRayHit hit)
{
    const float3 worldpos = origin + direction * hit.dist;
    float3 clipuvw;

    if (Test_WorldToPrevClipUVW(worldpos, clipuvw))
    {
        float2 deltacoord = abs(coord - (clipuvw.xy * pk_ScreenParams.xy));
        float rdepth = ViewDepth(clipuvw.z);
        float sdepth = SamplePreviousViewDepth(clipuvw.xy);
        float sviewz = -SamplePreviousViewNormal(clipuvw.xy).z + 0.15;

        bool isTexelOOB = dot(deltacoord, deltacoord) > 2.0f;
        bool isValidSurf = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
        bool isValidSky = hit.isMiss && !Test_DepthFar(sdepth);
        return isTexelOOB && (isValidSky || isValidSurf);
    }

    return false;
}

uint TraceRay_ScreenSpace(const float3 origin, const float3 direction, inout float hitDistance)
{
#if defined(PK_GI_SSRT_PRETRACE)
    float3 hitpos = 0.0f.xxx;
    const uint result = SreenSpaceRaymarch(origin, direction, hitpos);
    hitpos = SampleWorldPosition(hitpos.xy, ViewDepth(hitpos.z));
    hitDistance = max(0.0f, dot(normalize(direction), hitpos - origin));
    return result;
#else
    hitDistance = 0.0f;
    return RT_MISS;
#endif
}

#define TRACE_RAY(ORIGIN, DIRECTION, OUT_DIST, OUT_MISS)                                                                                        \
{                                                                                                                                               \
    const uint result = TraceRay_ScreenSpace(ORIGIN, DIRECTION, OUT_DIST);                                                                      \
    OUT_MISS = result != RT_HIT;                                                                                                                \
                                                                                                                                                \
    [[branch]]                                                                                                                                  \
    if (result == RT_MISS)                                                                                                                      \
    {                                                                                                                                           \
        traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, ORIGIN, OUT_DIST * 0.9f, DIRECTION, PK_GI_RAY_MAX_DISTANCE, 0);     \
        OUT_MISS = payload.hitDistance == 0xFFFFFFFFu;                                                                                          \
        OUT_DIST = uintBitsToFloat(payload.hitDistance);                                                                                        \
    }                                                                                                                                           \
}                                                                                                                                               \

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 raycoord = int2(gl_LaunchIDEXT.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(gl_LaunchIDEXT.xy);
    const float2 uv = (coord + 0.5f.xx) / size;
    float depth = SampleViewDepth(coord);
    
    GIRayParams params;
    GIRayHits hits;

    if (Test_DepthFar(depth))
    {
        GI_GET_RAY_PARAMS(coord, raycoord, depth, params)
        
        #if PK_GI_APPROX_ROUGH_SPEC == 1
        if (params.roughness >= PK_GI_MAX_ROUGH_SPEC)
        {
            hits.spec.dist = 1e+38f;
            hits.spec.isMiss = true;
            hits.spec.isScreen = false;
        }
        else
        #endif
        TRACE_RAY(params.origin, params.specdir, hits.spec.dist, hits.spec.isMiss)

        TRACE_RAY(params.origin, params.diffdir, hits.diff.dist, hits.diff.isMiss)

        {
            #if PK_GI_APPROX_ROUGH_SPEC == 1
            if (params.roughness < PK_GI_MAX_ROUGH_SPEC)
            #endif
            {
                hits.spec.dist = hits.spec.isMiss ? uint16BitsToHalf(0x7C00us) : hits.spec.dist;
                hits.spec.isScreen = IsScreenHit(coord, params.origin, params.specdir, hits.spec);
            }

            hits.diff.dist = hits.diff.isMiss ? uint16BitsToHalf(0x7C00us) : hits.diff.dist;
            hits.diff.isScreen = IsScreenHit(coord, params.origin, params.diffdir, hits.diff);
        }

        GI_Store_RayHits(raycoord, hits);
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
