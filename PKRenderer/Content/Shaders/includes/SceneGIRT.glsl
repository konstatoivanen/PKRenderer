#pragma once
#include "SceneGI.glsl"
#include "NoiseBlue.glsl"

PK_DECLARE_SET_SHADER uniform uimage2D pk_GI_RayHits;

struct GIRayHit { float dist; bool is_miss; bool is_screen; };
struct GIRayHits { GIRayHit diff; GIRayHit spec; uint diff_normal; };

bool GI_IsScreenHit(const float3 world_pos, bool is_miss)
{
    const float3 clip_uvw = WorldToClipUvwPrev(world_pos);

    if (Test_InUvw(clip_uvw))
    {
        const float depth_r = ViewDepth(clip_uvw.z);
        const float depth_s = SamplePreviousViewDepth(clip_uvw.xy);
        const float3 view_dir = UvToViewDir(clip_uvw.xy);
        const float3 view_nor = SamplePreviousViewNormal(clip_uvw.xy);
        const float sviewz = max(0.0f, dot(view_dir, -view_nor)) + 0.15;
        const bool is_valid_surf = abs(depth_s - depth_r) < (depth_r * 0.01f / sviewz);
        const bool is_valid_sky = is_miss && !Test_DepthIsScene(depth_s);
        return is_valid_sky || is_valid_surf;
    }

    return false;
}

bool GI_IsScreenHit(const int2 coord, const float3 world_pos, bool is_miss)
{
    const float3 clip_uvw = WorldToClipUvwPrev(world_pos);

    if (Test_InUvw(clip_uvw))
    {
        const float2 delta_coord = abs(coord - clip_uvw.xy * pk_ScreenParams.xy);
        const float depth_r = ViewDepth(clip_uvw.z);
        const float depth_s = SamplePreviousViewDepth(clip_uvw.xy);
        const float3 view_dir = UvToViewDir(clip_uvw.xy);
        const float3 view_nor = SamplePreviousViewNormal(clip_uvw.xy);
        const float sviewz = max(0.0f, dot(view_dir, -view_nor)) + 0.15;
        const bool is_texel_oob = dot(delta_coord, delta_coord) > 2.0f;
        const bool is_valid_surf = abs(depth_s - depth_r) < (depth_r * 0.01f / sviewz);
        const bool is_valid_sky = is_miss && !Test_DepthIsScene(depth_s);
        return is_texel_oob && (is_valid_sky || is_valid_surf);
    }

    return false;
}

float3 GI_GetRayViewOrigin(const int2 coord, float depth)
{
    return CoordToViewPos(coord, depth - depth * 1e-2f);
}

// Normal mapped normals often lead to self intersections.
// Apply a small bias away from surface.
float3 GI_GetRayOriginNormalOffset(const float3 normal, const float3 view_dir) 
{
    return normal * (0.01f / (saturate(-dot(view_dir, normal)) + 0.01f)) * 0.05f;
}

// Check if normal biased origin clips with geometry
float3 GI_ApplyNormalOffset(const float3 origin, const float3 normal, const float3 view_dir)
{
    const float3 offset = GI_GetRayOriginNormalOffset(normal, view_dir);
    const float3 uvw = WorldToClipUvw(origin + offset);
    const float z = PK_GI_SAMPLE_CLIP_DEPTH(uvw.xy);
    return origin + offset * step(z, uvw.z);
}

float2 GI_GetRayXi(int2 coord_ray)
{
    const float3 v = GlobalNoiseBlue(coord_ray + pk_GI_RayDither, pk_FrameIndex.y);
    return saturate(v.xy + ((v.z - 0.5f) / 256.0f));
}

#define GI_LOAD_RAY_PARAMS(COORD, RAYCOORD, DEPTH, NORMAL, ROUGHNESS)                 \
float3 origin = ViewToWorldPos(GI_GetRayViewOrigin(COORD, DEPTH));                    \
float3 view_dir = normalize(origin - pk_ViewWorldOrigin.xyz);                         \
origin = GI_ApplyNormalOffset(origin, NORMAL, view_dir);                              \
const float2 Xi = GI_GetRayXi(RAYCOORD);                                              \
float3 direction_diff = Fd_Inverse_Lambert(Xi, NORMAL);                               \
float3 direction_spec = Fr_Inverse_GGXVNDF_Full(Xi.yx, NORMAL, view_dir, ROUGHNESS);  \


GIRayHits GI_Load_RayHits(const int2 coord)
{
    uint2 packed = imageLoad(pk_GI_RayHits, coord).xy;
    const bool is_screen_diff = bitfieldExtract(packed.x, 15, 1) != 0;
    const bool is_screen_spec = bitfieldExtract(packed.x, 31, 1) != 0;
    packed.x &= 0x7FFF7FFFu; // Remove sign bits
    const bool is_miss_diff = bitfieldExtract(packed.x, 0, 16) == 0x7C00u;
    const bool is_miss_spec = bitfieldExtract(packed.x, 16, 16) == 0x7C00u;
    const float2 hit_t = unpackHalf2x16(packed.x);
    return GIRayHits(GIRayHit(hit_t.x, is_miss_diff, is_screen_diff), GIRayHit(hit_t.y, is_miss_spec, is_screen_spec), packed.y);
}

void GI_Store_RayHits(const int2 coord, const GIRayHits u)
{
    uint packed = packHalf2x16(float2(u.diff.dist, u.spec.dist));
    packed = u.diff.is_miss ? bitfieldInsert(packed, 0x7C00u, 0, 16) : packed;
    packed = u.spec.is_miss ? bitfieldInsert(packed, 0x7C00u, 16, 16) : packed;
    packed = bitfieldInsert(packed, u.diff.is_screen ? 0x1u : 0x0u, 15, 1);
    packed = bitfieldInsert(packed, u.spec.is_screen ? 0x1u : 0x0u, 31, 1);
    imageStore(pk_GI_RayHits, coord, uint4(packed, u.diff_normal, 0u, 0u));
}
