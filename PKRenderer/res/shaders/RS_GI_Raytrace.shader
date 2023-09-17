#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/Encoding.glsl
#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_SSRT_PRETRACE

struct TracePayload
{
    uint hitNormal;
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
        const float2 deltacoord = abs(coord - (clipuvw.xy * pk_ScreenParams.xy));
        const float rdepth = ViewDepth(clipuvw.z);
        const float sdepth = SamplePreviousViewDepth(clipuvw.xy);
        const float3 viewdir = normalize(UVToViewPos(clipuvw.xy, 1.0f));
        const float3 viewnor = SamplePreviousViewNormal(clipuvw.xy);
        const float sviewz = max(0.0f, dot(viewdir, -viewnor)) + 0.15;
        const bool isTexelOOB = dot(deltacoord, deltacoord) > 2.0f;
        const bool isValidSurf = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
        const bool isValidSky = hit.isMiss && !Test_DepthFar(sdepth);
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

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

void main()
{
    const int2 raycoord = int2(gl_LaunchIDEXT.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(gl_LaunchIDEXT.xy);
    const float depth = SampleViewDepth(coord);
    
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
        {
            const uint result = TraceRay_ScreenSpace(params.origin, params.specdir, hits.spec.dist);
            hits.spec.isMiss = result != RT_HIT;
            
            [[branch]]
            if (result == RT_MISS)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, params.origin, hits.spec.dist * 0.9f, params.specdir, PK_GI_RAY_TMAX, 0);
                hits.spec.isMiss = payload.hitDistance == 0xFFFFFFFFu;
                hits.spec.dist = uintBitsToFloat(payload.hitDistance);
            }
        }

        {
            const uint result = TraceRay_ScreenSpace(params.origin, params.diffdir, hits.diff.dist);
            hits.diff.isMiss = result != RT_HIT;

            [[branch]]
            if (result == RT_MISS)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, params.origin, hits.diff.dist * 0.9f, params.diffdir, PK_GI_RAY_TMAX, 0);
                hits.diff.isMiss = payload.hitDistance == 0xFFFFFFFFu;
                hits.diff.dist = uintBitsToFloat(payload.hitDistance);
            }
        }

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

        hits.diffNormal = payload.hitNormal;
        GI_Store_RayHits(raycoord, hits);
    }
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main() 
{
    payload.hitNormal = EncodeOctaUV(-gl_WorldRayDirectionEXT);
    payload.hitDistance = 0xFFFFFFFFu;
}

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

// @TODO replace this crap with this extention when it comes out of beta.
// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_ray_tracing_position_fetch.html
PK_DECLARE_READONLY_BUFFER(float3, pk_RT_Vertices, PK_SET_DRAW);
PK_DECLARE_READONLY_BUFFER(uint, pk_RT_Indices, PK_SET_DRAW);

void main()
{
    uint3 indices = uint3
    (
        PK_BUFFER_DATA(pk_RT_Indices, 3 * gl_PrimitiveID + 0),
        PK_BUFFER_DATA(pk_RT_Indices, 3 * gl_PrimitiveID + 1),
        PK_BUFFER_DATA(pk_RT_Indices, 3 * gl_PrimitiveID + 2)
    );

    float3 positions[3] =
    {
        PK_BUFFER_DATA(pk_RT_Vertices, indices[0]),
        PK_BUFFER_DATA(pk_RT_Vertices, indices[1]),
        PK_BUFFER_DATA(pk_RT_Vertices, indices[2])
    };

    float3 v0 = normalize(positions[1] - positions[0]);
    float3 v1 = normalize(positions[2] - positions[0]);
    float3 normal = cross(v0, v1);
    normal = mul(gl_ObjectToWorldEXT, float4(normal, 0.0f));

    if (dot(normal, gl_WorldRayDirectionEXT) > 0)
    {
        normal *= -1;
    }

    payload.hitNormal = EncodeOctaUV(normal);
    payload.hitDistance = floatBitsToUint(PK_GET_RAY_HIT_DISTANCE);
}
