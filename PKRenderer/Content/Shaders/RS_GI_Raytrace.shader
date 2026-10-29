
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_multi_compile _ PK_GI_SSRT_PRETRACE
#pragma pk_program RayGeneration MainRgs
#pragma pk_program RayMiss MainRms
#pragma pk_program RayClosestHit MainRchs

#include "includes/GBuffers.glsl"
#include "includes/SceneGI.glsl"
#include "includes/SceneGIRT.glsl"
#include "includes/Encoding.glsl"

struct RayPayload
{
    uint hitNormal;
    uint hitDistance;
};

bool TraceRay_ScreenSpace(const int2 coord, const float3 origin, const float3 direction, inout float hit_t)
{
#if defined(PK_GI_SSRT_PRETRACE)
    const float3 view_pos = WorldToViewPos(origin);
    const float3 view_dir = WorldToViewVec(direction);
    const float max_t = 0.05f * view_pos.z;
    const float2 resolution = pk_View.ScreenParams.xy;

    const float3 end = view_pos + view_dir * max_t;
    const float2 px = ViewToClipUv(end) * resolution + 0.5f.xx;
    const float samples = clamp(length(px - float2(coord)), 1.0f, 32.0f);

    const float threshold = 0.005f * sqrt(view_pos.z);
    float delta = max_t / samples;
    hit_t = delta;

    [[loop]]
    for (; hit_t < max_t; hit_t += delta)
    {
        const float3 s_pos = view_pos + view_dir * hit_t;
        const float2 s_uv = ViewToClipUv(s_pos);
        const int2 s_coord = int2(s_uv * resolution + 0.5f.xx);
        const float s_depth = SampleViewDepth(s_coord);
        const float s_depth_delta = s_pos.z - s_depth;
        const bool s_in_screen = Test_InUv(s_uv);

        delta *= 1.04f;

        if (s_depth_delta > threshold || !s_in_screen)
        {
            return s_depth_delta < 5.0f * threshold && s_in_screen;
        }
    }

    return false;
#else
    hit_t = 0.0f;
    return false;
#endif
}

void MainRgs()
{
    const uint2 launchId = DispatchRaysIndex().xy;
    const int2 coord_ray = int2(launchId);
    const int2 coord = GI_ExpandCheckerboardCoord(launchId);
    const float depth = PK_GI_SAMPLE_DEPTH(coord);

    GIRayHits hits;
    hits.spec.dist = 1e+38f;
    hits.spec.is_miss = true;
    hits.spec.is_screen = false;

    if (Test_DepthIsScene(depth))
    {
        const float4 normal_roughness = SampleWorldNormalRoughness(coord);
        GI_LOAD_RAY_PARAMS(coord, coord_ray, depth, normal_roughness.xyz, normal_roughness.w)

        RayPayload payload = { 0u, 0xFFFFFFFFu };
        RayDesc ray;
        ray.Origin = origin;
        ray.TMin = 0.0f;
        ray.TMax = PK_GI_RAY_TMAX;

        #if PK_GI_APPROX_ROUGH_SPEC == 1
        if (normal_roughness.w < PK_GI_MAX_ROUGH_SPEC)
        #endif
        {
            hits.spec.is_screen = TraceRay_ScreenSpace(coord, origin, direction_spec, hits.spec.dist);
            hits.spec.is_miss = !hits.spec.is_screen;

            [[branch]]
            if (hits.spec.is_miss)
            {
                ray.Direction = direction_spec;
                TraceRay(pk_SceneStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, ray, payload);
                hits.spec.is_miss = payload.hitDistance == 0xFFFFFFFFu;
                hits.spec.dist = asfloat(payload.hitDistance);
            }
        }

        {
            hits.diff.is_screen = TraceRay_ScreenSpace(coord, origin, direction_diff, hits.diff.dist);
            hits.diff.is_miss = !hits.diff.is_screen;

            [[branch]]
            if (hits.diff.is_miss)
            {
                ray.Direction = direction_diff;
                TraceRay(pk_SceneStructure, RAY_FLAG_FORCE_OPAQUE, 0xff, 0, 0, 0, ray, payload);
                hits.diff.is_miss = payload.hitDistance == 0xFFFFFFFFu;
                hits.diff.dist = asfloat(payload.hitDistance);
            }
        }

        #if PK_GI_APPROX_ROUGH_SPEC == 1
        if (normal_roughness.w < PK_GI_MAX_ROUGH_SPEC)
        #endif
        {
            hits.spec.dist = hits.spec.is_miss ? PK_HALF_MAX_MINUS1 : hits.spec.dist;

            [[branch]]
            if (!hits.spec.is_screen)
            {
                hits.spec.is_screen = GI_IsScreenHit(coord, origin + direction_spec * hits.spec.dist, hits.spec.is_miss);
            }
        }

        {
            hits.diff.dist = hits.diff.is_miss ? PK_HALF_MAX_MINUS1 : hits.diff.dist;
            hits.diff_normal = payload.hitNormal;

            [[branch]]
            if (!hits.diff.is_screen)
            {
                hits.diff.is_screen = GI_IsScreenHit(coord, origin + direction_diff * hits.diff.dist, hits.diff.is_miss);
            }

            [[branch]]
            if (hits.diff.is_screen)
            {
                const float3 clip_uvw = WorldToClipUvw(origin + direction_diff * hits.diff.dist);
                hits.diff_normal = EncodeOctaUv2x16(SampleWorldNormal(clip_uvw.xy));
            }
        }

        GI_Store_RayHits(coord_ray, hits);
    }
}

void MainRms(inout RayPayload payload)
{
    payload.hitNormal = EncodeOctaUv2x16(-WorldRayDirection());
    payload.hitDistance = 0xFFFFFFFFu;
}

void MainRchs(inout RayPayload payload, BuiltInTriangleIntersectionAttributes attrib)
{
    const float3 p0 = HitTriangleVertexPosition(0);
    const float3 p1 = HitTriangleVertexPosition(1);
    const float3 p2 = HitTriangleVertexPosition(2);
    const float3 v0 = normalize(p1 - p0);
    const float3 v1 = normalize(p2 - p0);

    float3 normal = cross(v0, v1);
    normal = mul(ObjectToWorld3x4(), float4(normal, 0.0f));

    if (dot(normal, WorldRayDirection()) > 0.0f)
    {
        normal = -normal;
    }

    payload.hitNormal = EncodeOctaUv2x16(normal);
    payload.hitDistance = asuint(RayTCurrent());
}
