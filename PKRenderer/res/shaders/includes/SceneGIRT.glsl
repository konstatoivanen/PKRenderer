#pragma once

#include SceneGI.glsl
#include NoiseBlue.glsl

layout(rg32ui, set = PK_SET_SHADER) uniform uimage2D pk_GI_RayHits;

struct GIRayHit { float dist; bool isMiss; bool isScreen; };
struct GIRayHits { GIRayHit diff; GIRayHit spec; uint diffNormal; };

bool GI_IsScreenHit(const float3 worldpos, bool isMiss)
{
    const float3 clipuvw = WorldToClipUVWPrev(worldpos);

    if (Test_InUVW(clipuvw))
    {
        const float rdepth = ViewDepth(clipuvw.z);
        const float sdepth = SamplePreviousViewDepth(clipuvw.xy);
        const float3 viewdir = normalize(UVToViewPos(clipuvw.xy, 1.0f));
        const float3 viewnor = SamplePreviousViewNormal(clipuvw.xy);
        const float sviewz = max(0.0f, dot(viewdir, -viewnor)) + 0.15;
        const bool isValidSurf = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
        const bool isValidSky = isMiss && !Test_DepthFar(sdepth);
        return isValidSky || isValidSurf;
    }

    return false;
}

bool GI_IsScreenHit(const int2 coord, const float3 worldpos, bool isMiss)
{
    const float3 clipuvw = WorldToClipUVWPrev(worldpos);

    if (Test_InUVW(clipuvw))
    {
        const float2 deltacoord = abs(coord - clipuvw.xy * pk_ScreenParams.xy);
        const float rdepth = ViewDepth(clipuvw.z);
        const float sdepth = SamplePreviousViewDepth(clipuvw.xy);
        const float3 viewdir = normalize(UVToViewPos(clipuvw.xy, 1.0f));
        const float3 viewnor = SamplePreviousViewNormal(clipuvw.xy);
        const float sviewz = max(0.0f, dot(viewdir, -viewnor)) + 0.15;
        const bool isTexelOOB = dot(deltacoord, deltacoord) > 2.0f;
        const bool isValidSurf = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);
        const bool isValidSky = isMiss && !Test_DepthFar(sdepth);
        return isTexelOOB && (isValidSky || isValidSurf);
    }

    return false;
}

float3 GI_GetRayViewOrigin(const int2 coord, float depth)
{
    return CoordToViewPos(coord, depth - depth * 1e-2f);
}

float3 GI_GetRayOriginNormalOffset(const float3 normal, const float3 viewdir) 
{
    return normal * (0.01f / (saturate(-dot(viewdir, normal)) + 0.01f)) * 0.05f;
}

float2 GI_GetRayXi(int2 raycoord)
{
    const float3 v = GlobalNoiseBlue(raycoord + pk_GI_RayDither, pk_FrameIndex.y);
    return saturate(v.xy + ((v.z - 0.5f) / 256.0f));
}

#define GI_LOAD_RAY_PARAMS(COORD, RAYCOORD, DEPTH, NORMAL, ROUGHNESS)           \
float3 origin = ViewToWorldPos(GI_GetRayViewOrigin(COORD, DEPTH));              \
float3 viewdir = normalize(origin - pk_ViewWorldOrigin.xyz);                    \
origin += GI_GetRayOriginNormalOffset(NORMAL, viewdir);                         \
const float2 Xi = GI_GetRayXi(RAYCOORD);                                        \
float3 directionDiff = Fd_Inverse_Lambert(Xi, NORMAL);                          \
float3 directionSpec = Fr_Inverse_GGXVNDF(Xi.yx, NORMAL, viewdir, ROUGHNESS);   \


GIRayHits GI_Load_RayHits(const int2 coord)
{
    uint2 packed = imageLoad(pk_GI_RayHits, coord).xy;
    const bool isScreenDiff = bitfieldExtract(packed.x, 15, 1) != 0;
    const bool isScreenSpec = bitfieldExtract(packed.x, 31, 1) != 0;
    packed.x &= 0x7FFF7FFFu; // Remove sign bits
    const bool isMissDiff = bitfieldExtract(packed.x, 0, 16) == 0x7C00u;
    const bool isMissSpec = bitfieldExtract(packed.x, 16, 16) == 0x7C00u;
    const float2 hitDist = unpackHalf2x16(packed.x);
    return GIRayHits(GIRayHit(hitDist.x, isMissDiff, isScreenDiff), GIRayHit(hitDist.y, isMissSpec, isScreenSpec), packed.y);
}

void GI_Store_RayHits(const int2 coord, const GIRayHits u)
{
    uint packed = packHalf2x16(float2(u.diff.dist, u.spec.dist));
    packed = u.diff.isMiss ? bitfieldInsert(packed, 0x7C00u, 0, 16) : packed;
    packed = u.spec.isMiss ? bitfieldInsert(packed, 0x7C00u, 16, 16) : packed;
    packed = bitfieldInsert(packed, u.diff.isScreen ? 0x1u : 0x0u, 15, 1);
    packed = bitfieldInsert(packed, u.spec.isScreen ? 0x1u : 0x0u, 31, 1);
    imageStore(pk_GI_RayHits, coord, uint4(packed, u.diffNormal, 0u, 0u));
}