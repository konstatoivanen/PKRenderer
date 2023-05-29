#version 460
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

struct TracePayload
{
    float hitDistance;
    bool isMiss;
};

#pragma PROGRAM_RAY_GENERATION

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

bool TraceRay(const float3 origin, const float3 direction, inout float hitDistance)
{
    payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
    traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, 0.0f, direction, PK_GI_RAY_MAX_DISTANCE, 0);
    hitDistance = payload.hitDistance;
    return payload.isMiss;
}

void main()
{
    int2 size = imageSize(pk_GI_RayHits).xy;
    int2 coord = int2(gl_LaunchIDEXT.xy);
    float depth = SampleLinearDepth(coord);

    if (depth >= pk_ProjectionParams.z - 1e-4f)
    {
        imageStore(pk_GI_RayHits, coord, uint4(0xFFFFFFFF));
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

    GIRayDirections directions = GI_GetRayDirections(coord, N, V, NR.w);
    
    GIRayHits hits;
    hits.isMissDiff = TraceRay(O, directions.diff, hits.distDiff);
    hits.isMissSpec = TraceRay(O, directions.spec, hits.distSpec);
    GI_Store_RayHits(coord, hits);
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

// Do nothing for now
void main() { payload.isMiss = true; }

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
    payload.isMiss = false;
    payload.hitDistance = PK_GET_RAY_HIT_DISTANCE;
}
