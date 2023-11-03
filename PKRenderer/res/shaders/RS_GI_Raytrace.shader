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

bool TraceRay_ScreenSpace(const int2 coord, const float3 origin, const float3 direction, inout float hitT)
{
#if defined(PK_GI_SSRT_PRETRACE)
    const float3 viewpos = WorldToViewPos(origin);
    const float3 viewdir = WorldToViewDir(direction);
    const float maxt = 0.05f * viewpos.z;
    const float2 dims = pk_ScreenParams.xy;

    float samples;
    {
        const float3 end = viewpos + viewdir * maxt;
        const float2 px = ViewToClipUV(end) * dims + 0.5f.xx;
        samples = length(px - float2(coord));
        samples = clamp(samples, 1.0f, 32.0f);
    }

    const float threshold = 0.005f * sqrt(viewpos.z);
    float delta = maxt / samples;
    hitT = delta;

    [[loop]]
    for (; hitT < maxt; hitT += delta)
    {
        const float3 samplepos = viewpos + viewdir * hitT;
        const float2 uv = ViewToClipUV(samplepos);
        const int2 scoord = int2(uv * dims + 0.5f.xx);
        const float depth = SampleViewDepth(scoord);
        const float depthDelta = samplepos.z - depth;
        const bool inScreen = Test_InUV(uv);

        delta *= 1.04f;

        if (depthDelta > threshold || !inScreen)
        {
            return depthDelta < 5.0 * threshold && inScreen;
        }
    }

    return false;

#else
    hitT = 0.0f;
    return false;
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
            hits.spec.isScreen = TraceRay_ScreenSpace(coord, origin, directionSpec, hits.spec.dist);
            hits.spec.isMiss = !hits.spec.isScreen;
            
            [[branch]]
            if (hits.spec.isMiss)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, directionSpec, PK_GI_RAY_TMAX, 0);
                hits.spec.isMiss = payload.HIT_DISTANCE == 0xFFFFFFFFu;
                hits.spec.dist = uintBitsToFloat(payload.HIT_DISTANCE);
            }
        }

        {
            hits.diff.isScreen = TraceRay_ScreenSpace(coord, origin, directionDiff, hits.diff.dist);
            hits.diff.isMiss = !hits.diff.isScreen;

            [[branch]]
            if (hits.diff.isMiss)
            {
                traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, directionDiff, PK_GI_RAY_TMAX, 0);
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
                
                [[branch]]
                if (!hits.spec.isScreen)
                {
                    hits.spec.isScreen = GI_IsScreenHit(coord, origin + directionSpec * hits.spec.dist, hits.spec.isMiss);
                }
            }

            hits.diff.dist = hits.diff.isMiss ? uint16BitsToHalf(0x7C00us) : hits.diff.dist;

            [[branch]]
            if (!hits.spec.isScreen)
            {
                hits.diff.isScreen = GI_IsScreenHit(coord, origin + directionDiff * hits.diff.dist, hits.diff.isMiss);
            }
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
