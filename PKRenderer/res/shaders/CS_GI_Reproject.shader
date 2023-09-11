#version 460
#pragma PROGRAM_COMPUTE

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include includes/SceneGIFiltering.glsl

#multi_compile _ PK_GI_SPEC_VIRT_REPROJECT

void WriteMipMask(const int2 coord, const GIDiff diff, const GISpec spec)
{
    const int2 base = ((coord + 8) >> 4) - 1;
    
    [[branch]]
    if (diff.history < 16 || spec.history < 16)
    {
        imageStore(pk_GI_ScreenDataMipMask, base + int2(0, 0), uint4(1u));
        imageStore(pk_GI_ScreenDataMipMask, base + int2(1, 0), uint4(1u));
        imageStore(pk_GI_ScreenDataMipMask, base + int2(0, 1), uint4(1u));
        imageStore(pk_GI_ScreenDataMipMask, base + int2(1, 1), uint4(1u));
    }
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float depth = SampleViewDepth(coord);

    // Far clip or new backbuffer
    if (pk_FrameIndex.y == 0u || !Test_DepthFar(depth))
    {
        GI_Store_Packed_Diff(coord, uint4(0));
        GI_Store_Packed_Spec(coord, uint2(0));
        return;
    }

    GIDiff diff = pk_Zero_GIDiff;
    GISpec spec = pk_Zero_GISpec;
    GISpec specVirtual = pk_Zero_GISpec;
    float wSumDiff = 0.0f;
    float wSumSpec = 0.0f;
    float wSumVSpec = 0.0f;
    float antilagSpec = 1.0f;
    float antilagDiff = 1.0f;

    // Filters
    {
        const float4 normalroughness = SampleViewNormalRoughness(coord);
        const float3 normal = normalroughness.xyz;
        const float roughness = normalroughness.w;
        const float depthBias = lerp(0.1f, 0.01f, -normal.z);
        const float3 viewpos = SampleViewPosition(coord, size, depth);
        const float3 viewdir = normalize(viewpos);
        const float nv = dot(normal, -viewdir);
        const float parallax = GI_GetParallax(viewdir, normalize(viewpos - pk_ViewSpaceCameraDelta.xyz));
        const float2 screenuvPrev = GI_ViewToPrevScreenUV(viewpos);
        const int2 coordPrev = int2(screenuvPrev);

        antilagSpec = GI_GetAntilagSpecular(roughness, nv, parallax);

        // Reconstruct diff & naive spec
        GI_SFLT_REPRO_BILINEAR(screenuvPrev, coordPrev, normal, depth, depthBias, roughness, wSumDiff, wSumSpec, diff, spec)

        #if defined(PK_GI_SPEC_VIRT_REPROJECT)
        if (!Test_EPS6(wSumSpec))
        {
            const float virtualDist = (spec.ao / wSumSpec) * PK_GI_RAY_MAX_DISTANCE * GI_GetSpecularDominantFactor(nv, sqrt(roughness));
            GI_SFLT_REPRO_VIRTUAL_SPEC(viewpos, viewdir, normal, depth, roughness, virtualDist, wSumVSpec, specVirtual)
        }
        #endif

        // Reduce diff antilag on poor reproject.
        antilagDiff = lerp(0.1f, 1.0f, saturate(wSumDiff / 0.25f));
    }

    // Normalization
    {
        diff.history = clamp((diff.history / wSumDiff) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilagDiff);
        diff = GI_Mul_NoHistory(diff, 1.0f / wSumDiff);

        spec.history = clamp((spec.history / wSumSpec) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilagSpec);
        spec = GI_Mul_NoHistory(spec, 1.0f / wSumSpec);

        // Get min of virtual reprojected spec & naive spec to eliminate ghosting.
        if (!Test_EPS6(wSumVSpec))
        {
            specVirtual.history = clamp((specVirtual.history / wSumVSpec) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilagSpec);
            specVirtual = GI_Mul_NoHistory(specVirtual, 1.0f / wSumVSpec);

            spec.history = min(specVirtual.history, spec.history);
            spec.radiance = min(spec.radiance, specVirtual.radiance);
            spec.ao = min(spec.ao, specVirtual.ao);
        }
    }

    const bool invalidDiff = Test_EPS6(wSumDiff) || Any_IsNaN(diff.sh.Y) || Any_IsNaN(diff.sh.CoCg) || isnan(diff.ao) || isnan(diff.history);
    const bool invalidSpec = Test_EPS6(wSumSpec) || Any_IsNaN(spec.radiance) || isnan(spec.ao) || isnan(spec.history);
    GI_Store_Packed_Diff(coord, invalidDiff ? uint4(0) : GI_Pack_Diff(diff));
    GI_Store_Packed_Spec(coord, invalidSpec ? uint2(0) : GI_Pack_Spec(spec));
    WriteMipMask(coord, diff, spec);
}