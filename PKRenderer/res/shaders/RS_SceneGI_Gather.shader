#version 460
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

struct TracePayload
{
    float hitDistance;
};

#pragma PROGRAM_RAY_GENERATION

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

float TraceRay(const float3 origin, const float3 direction)
{
    payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
    traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, direction, PK_GI_RAY_MAX_DISTANCE, 0);
    return payload.hitDistance;
}

void main()
{
    int2 size = imageSize(pk_ScreenGI_Hits).xy;
    int2 coord = int2(gl_LaunchIDEXT.xy);
    float depth = SampleLinearDepth(coord);

    if (depth >= pk_ProjectionParams.z - 1e-4f)
    {
        imageStore(pk_ScreenGI_Hits, coord, float4(PK_GI_RAY_MIN_DISTANCE, PK_GI_RAY_MIN_DISTANCE, 0.0f.xx));
        return;
    }

    // Apply bias to avoid rays clipping with geo at high distances
    depth -= depth * 1e-2f; 

    const float4 NR = SampleWorldNormalRoughness(coord);
    const float3 N = NR.xyz;

    // @TODO HACK: Due to normal mapping the ray could intersect immediately, causing a feedback loop.
    // Offsetting origin by min distance fixes this, but causes incorrect near hits.
    const float3 O = SampleWorldPosition(coord, size, depth) + N * PK_GI_RAY_MIN_DISTANCE;
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
    const float2 Xi = GetSampleOffset(coord, pk_FrameIndex);

    const float3 dirDiff = ImportanceSampleLambert(Xi, N);
    const float3 dirSpec = ImportanceSampleSmithGGX(Xi, N, V, NR.w);

    const float distanceDiff = TraceRay(O, dirDiff);
    const float distanceSpec = TraceRay(O, dirSpec);

    imageStore(pk_ScreenGI_Hits, coord, float4(distanceDiff, distanceSpec, 0.0f.xx));
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
    // Do nothing for now
}

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
    payload.hitDistance = PK_GET_RAY_HIT_DISTANCE;
}
