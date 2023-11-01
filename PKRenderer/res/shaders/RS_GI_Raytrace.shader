#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#include includes/GBuffers.glsl
#include includes/SceneGI.glsl
#include includes/SceneGIRT.glsl
#include includes/Encoding.glsl

#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_SSRT_PRETRACE

#define HIT_NORMAL x
#define HIT_DISTANCE y

#pragma PROGRAM_RAY_GENERATION
#define STEP_COUNT 32u
#define MAX_MIP 8s
#define HIT_TOLERANCE -0.01f
#define RT_MISS 0u
#define RT_HIT 1u
#define RT_SKY 2u

float SreenSpaceRaymarch(float3 origin, float3 direction, uint2 uv0)
{
    float3 p0 = WorldToViewPos(origin);
    float3 ray = WorldToViewDir(direction);
    float maxt = 0.05 * p0.z; // 1m -> 5cm, 100m -> 5m
    float2 dims = pk_ScreenParams.xy;
    uint2 pixel_crd = uint2(0);

    float samples;
    {
        float3 p1 = p0 + ray * maxt;
        float3 clip1 = ViewToClipPos(p1).xyw;

        float2 uv1 = clip1.xy / clip1.z;
        uv1 = saturate(uv1 * float2(0.5, -0.5) + 0.5);
        uv1 = uv1 * dims + 0.5;

        samples = length(uv1 - float2(uv0));
        samples = clamp(samples, 1.0, 32.0);
    }

    float delta = maxt / samples;
    float t = delta;

    float threshold = 0.005f * sqrt(p0.z); // 1m -> 0.5cm; 100m -> 5cm

    [[loop]]
    for (; t < maxt; t += delta)
    {
        float4 p = float4(p0 + ray * t, 1.0);
        float3 pc = ViewToClipPos(p.xyz).xyw;
        float2 uv = pc.xy / pc.z;
        uv = uv * float2(0.5, -0.5) + 0.5;

        pixel_crd = uint2(uv * dims + 0.5);

        float z_traced = SampleViewDepth(pixel_crd);
        float dz = p.z - z_traced;

        bool outside = !Test_InUV(uv);

        // increasing step...
        delta *= 1.04;

        if (dz > threshold || outside)
        {
            return (dz < 5.0 * threshold && !outside) ? t : 0.0f;
        }
    }

    return 0.0f;
}

uint TraceRay_ScreenSpace(const int2 coord, const float3 origin, const float3 direction, inout float hitDistance)
{
#if defined(PK_GI_SSRT_PRETRACE)
    hitDistance = SreenSpaceRaymarch(origin, direction, coord);
    return hitDistance == 0.0f ? RT_MISS : RT_HIT;
#else
    hitDistance = 0.0f;
    return RT_MISS;
#endif
}

PK_DECLARE_RT_PAYLOAD_OUT(uint2, payload, 0);

void main()
{
    const int2 raycoord = int2(gl_LaunchIDEXT.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(gl_LaunchIDEXT.xy);
    const float depth = PK_GI_SAMPLE_DEPTH(coord);
    
    GIRayHits hits;

    if (Test_DepthFar(depth))
    {
        const float4 normalRoughness = SampleWorldNormalRoughness(coord);

        GI_LOAD_RAY_PARAMS(coord, raycoord, depth, normalRoughness.xyz, normalRoughness.w)
        
        #if PK_GI_APPROX_ROUGH_SPEC == 1
        if (normalRoughness.w >= PK_GI_MAX_ROUGH_SPEC)
        {
            hits.spec.dist = 1e+38f;
            hits.spec.isMiss = true;
            hits.spec.isScreen = false;
        }
        else
        #endif
        {
            const uint result = TraceRay_ScreenSpace(coord, origin, directionSpec, hits.spec.dist);
            hits.spec.isMiss = result != RT_HIT;
            
            [[branch]]
            if (result == RT_MISS)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, hits.spec.dist * 0.9f, directionSpec, PK_GI_RAY_TMAX, 0);
                hits.spec.isMiss = payload.HIT_DISTANCE == 0xFFFFFFFFu;
                hits.spec.dist = uintBitsToFloat(payload.HIT_DISTANCE);
            }
        }

        {
            const uint result = TraceRay_ScreenSpace(coord, origin, directionDiff, hits.diff.dist);
            hits.diff.isMiss = result != RT_HIT;

            [[branch]]
            if (result == RT_MISS)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, hits.diff.dist * 0.9f, directionDiff, PK_GI_RAY_TMAX, 0);
                hits.diff.isMiss = payload.HIT_DISTANCE == 0xFFFFFFFFu;
                hits.diff.dist = uintBitsToFloat(payload.HIT_DISTANCE);
            }
        }

        {
            #if PK_GI_APPROX_ROUGH_SPEC == 1
            if (normalRoughness.w < PK_GI_MAX_ROUGH_SPEC)
            #endif
            {
                hits.spec.dist = hits.spec.isMiss ? uint16BitsToHalf(0x7C00us) : hits.spec.dist;
                hits.spec.isScreen = GI_IsScreenHit(coord, origin + directionSpec * hits.spec.dist, hits.spec.isMiss);
            }

            hits.diff.dist = hits.diff.isMiss ? uint16BitsToHalf(0x7C00us) : hits.diff.dist;
            hits.diff.isScreen = GI_IsScreenHit(coord, origin + directionDiff * hits.diff.dist, hits.diff.isMiss);
        }

        hits.diffNormal = payload.HIT_NORMAL;
        GI_Store_RayHits(raycoord, hits);
    }
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(uint2, payload, 0);

void main() 
{
    payload.HIT_NORMAL = EncodeOctaUV(-gl_WorldRayDirectionEXT);
    payload.HIT_DISTANCE = 0xFFFFFFFFu;
}

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(uint2, payload, 0);

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
    normal = gl_ObjectToWorldEXT * float4(normal, 0.0f);

    if (dot(normal, gl_WorldRayDirectionEXT) > 0)
    {
        normal *= -1;
    }

    payload.HIT_NORMAL = EncodeOctaUV(normal);
    payload.HIT_DISTANCE = floatBitsToUint(PK_GET_RAY_HIT_DISTANCE);
}
