#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_SPEC_VIRT_REPROJECT
#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include includes/SceneGIFiltering.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);

#if defined(PK_GI_CHECKERBOARD_TRACE)
    const int2 storeCoord = int2
        (
            coord.x / 2 + int(GI_GetCheckerboardOffset(coord, pk_FrameIndex.y) * (pk_ScreenSize.x / 2)),
            coord.y
            );
#else
    const int2 storeCoord = coord;
#endif

    const float depth = SampleViewDepth(coord);

    // Far clip or new backbuffer
    if (pk_FrameIndex.y == 0u || !Test_DepthFar(depth))
    {
        GI_Store_Packed_Diff(storeCoord, uint4(0));
        GI_Store_Packed_Spec(storeCoord, uint2(0));
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
    float deltaNdotV = 0.0f;
    bool discardSpec = false;

    // Filters
    {
        const float4 normalroughness = SampleViewNormalRoughness(coord);
        const float3 normal = normalroughness.xyz;
        const float roughness = normalroughness.w;
        const float depthBias = lerp(0.1f, 0.01f, -normal.z);
        const float3 viewpos = SampleViewPosition(coord, depth);
        const float3 viewdir = normalize(viewpos);
        const float nv = dot(normal, -viewdir);
        const float parallax = GI_GetParallax(viewdir, normalize(viewpos - pk_ViewSpaceCameraDelta.xyz));
        const float2 screenuvPrev = GI_ViewToPrevScreenUV(viewpos);
        const int2 coordPrev = int2(screenuvPrev);

#if PK_GI_APPROX_ROUGH_SPEC == 1
        discardSpec = roughness >= PK_GI_MAX_ROUGH_SPEC;
#endif
        // Reconstruct diff & naive spec
        GI_SF_REPRO_BILINEAR(screenuvPrev, coordPrev, normal, depth, depthBias, roughness, wSumDiff, wSumSpec, diff, spec)

        // Reduce diff antilag on poor reproject.
        antilagDiff = lerp(0.1f, 1.0f, saturate(wSumDiff / 0.25f));
        antilagSpec = GI_GetAntilagSpecular(roughness, nv, parallax);

#if defined(PK_GI_SPEC_VIRT_REPROJECT)
        if (!Test_EPS6(wSumSpec) && !discardSpec)
        {
            const float virtualDist = (spec.ao / wSumSpec) * PK_GI_RAY_TMAX * GI_GetSpecularDominantFactor(nv, sqrt(roughness));
            GI_SF_REPRO_VIRTUAL_SPEC(viewpos, viewdir, normal, depth, roughness, virtualDist, wSumVSpec, specVirtual)
        }
#endif
    }

    // Normalization
    {
        diff.history = clamp(diff.history / wSumDiff, 0.0f, PK_GI_DIFF_MAX_HISTORY * antilagDiff);
        spec.history = clamp(spec.history / wSumSpec, 0.0f, PK_GI_SPEC_MAX_HISTORY * antilagSpec);

        diff = GI_Mul_NoHistory(diff, 1.0f / wSumDiff);
        spec = GI_Mul_NoHistory(spec, 1.0f / wSumSpec);
        
        // Get min of virtual reprojected spec & naive spec to eliminate ghosting.
        if (!Test_EPS6(wSumVSpec))
        {
            specVirtual.history = clamp(specVirtual.history / wSumVSpec, 0.0f, PK_GI_SPEC_MAX_HISTORY * antilagSpec);
            specVirtual = GI_Mul_NoHistory(specVirtual, 1.0f / wSumVSpec);

            spec.history = min(specVirtual.history, spec.history);
            spec.radiance = min(spec.radiance, specVirtual.radiance);
            spec.ao = min(spec.ao, specVirtual.ao);
        }
    }

    const bool invalidDiff = Test_EPS6(wSumDiff) || Any_IsNaN(diff.sh.Y) || Any_IsNaN(diff.sh.CoCg) || isnan(diff.ao) || isnan(diff.history);
    const bool invalidSpec = Test_EPS6(wSumSpec) || Any_IsNaN(spec.radiance) || isnan(spec.ao) || isnan(spec.history);

    GI_Store_Packed_Diff(storeCoord, invalidDiff ? uint4(0) : GI_Pack_Diff(diff));
    GI_Store_Packed_Spec(storeCoord, invalidSpec ? uint2(0) : GI_Pack_Spec(spec));

    {
        const int2 base = ((coord + 8) >> 4) - 1;

        [[branch]]
        if (diff.history < PK_GI_HISTORY_FILL_THRESHOLD || invalidDiff ||
           (spec.history < PK_GI_HISTORY_FILL_THRESHOLD && !discardSpec))
        {
            imageStore(pk_GI_PackedMipMask, base + int2(0, 0), uint4(1u));
            imageStore(pk_GI_PackedMipMask, base + int2(1, 0), uint4(1u));
            imageStore(pk_GI_PackedMipMask, base + int2(0, 1), uint4(1u));
            imageStore(pk_GI_PackedMipMask, base + int2(1, 1), uint4(1u));
        }
    }
}

/*
layout(rg32ui, set = PK_SET_DRAW) uniform uimage2D pk_Gradients;

#define GRADIENT_STRATA_SIZE 3

void GradientReproject(int2 coord)
{
    if (coord.x * GRADIENT_STRATA_SIZE >= pk_ScreenSize.x || coord.y * GRADIENT_STRATA_SIZE >= pk_ScreenSize.y)
    {
        return;
    }

    const uint2 prevPos = imageLoad(pk_Gradients, coord).y;

    int2 prevShadingPix = int2(0);
    uint curStrataPos = 0;
    int2 curShadingPix = int2(0);
    float2 prevLuminance = 0.0f.xx;

    for (uint yy = 0u; yy < GRADIENT_STRATA_SIZE; ++yy)
    for (uint xx = 0u; xx < GRADIENT_STRATA_SIZE; ++xx)
    {
        if (yy * GRADIENT_STRATA_SIZE + xx == prevPos)
        {
            continue;
        }

        const int2 iter_curPix = coord * COMPUTE_ASVGF_STRATA_SIZE + int2(xx, yy);
        const int2 iter_prevPix = ivec2(floor(getPrevScreenPos(framebufMotion_Sampler, iter_curPix)));
        const float iter_prevLuminance = imageLoad(framebufGradientInputs_Prev, iter_prevPix).x;

        if (max(iter_prevLuminance.x, iter_prevLuminance.y) > max(prevLuminance.x, prevLuminance.y))
        {
            // (b) prevShadingPix is a forward-projected pixel of curShadingPix
            prevShadingPix = iter_prevPix;
            curShadingPix = iter_curPix;
            curStrataPos = yy * COMPUTE_ASVGF_STRATA_SIZE + xx;
            // (e) luminance of a shading sample from previous frame
            prevLuminance = iter_prevLuminance;
        }
    }

    if (!testPixInRenderArea(prevShadingPix, getCheckerboardedRenderArea(curShadingPix)) ||
        isSkyPix(curShadingPix) ||
        (prevLuminance.x <= 0.0 && prevLuminance.y <= 0.0))
    {
        imageStore(framebufDISPingGradient, gradPix, vec4(0.0));
        imageStore(framebufGradientPrevPix, gradPix, uvec4(COMPUTE_ASVGF_STRATA_SIZE * COMPUTE_ASVGF_STRATA_SIZE + 1));
        return;
    }

    // (f) get shading sample from current frame
    const uint oldSeed = getRandomSeed(prevShadingPix, globalUniform.frameId - 1);

    Surface prevSurf = fetchGbufferSurface_NoAlbedoViewDir_Prev(prevShadingPix);

    // get exact position from visibility buffer, and fix up prevSurf
    // to account subpix imprecision of G-buffer's surfacePosition
    const vec4 visBufPrev = texelFetch(framebufVisibilityBuffer_Prev_Sampler, prevShadingPix, 0);
    const bool matchedSurface = unpackPrevVisibilityBuffer(visBufPrev, prevSurf.position);
    prevSurf.toViewerDir = normalize(globalUniform.cameraPositionPrev.xyz - prevSurf.position);

    vec2 forwardProjectedLuminance = vec2(0.0);

    if (matchedSurface)
    {
        Reservoir prevReservoir = imageLoadReservoir_Prev(prevShadingPix);

        if (isReservoirValid(prevReservoir))
        {
            prevReservoir.selected = lightSources_Index_PrevToCur[prevReservoir.selected];

            if (prevReservoir.selected != UINT32_MAX)
            {
                vec3 diffuse, specular;
                processDirectIllumination(oldSeed, prevSurf, prevReservoir, diffuse, specular);

                forwardProjectedLuminance = vec2(getLuminance(diffuse), getLuminance(specular));
            }
        }
    }

    vec3 gradDIS = vec3(0.0);

    const float gradSample = forwardProjectedLuminance.x - prevLuminance.x;
    const float normFactor = max(forwardProjectedLuminance.x, prevLuminance.x);
    const float gradient = getAntilagAlpha(gradSample, normFactor);

    // smooth temporally, to make antilag less aggressive over a small period of time
    {
        const ivec2 gradPixPrev = prevShadingPix / COMPUTE_ASVGF_STRATA_SIZE;

        const vec3 gradDISPrev = imageLoad(framebufDISGradientHistory, gradPixPrev).xyz;
        gradDIS = mix(gradient, gradDISPrev, 0.5);
    }

    imageStore(framebufDISPingGradient, gradPix, vec4(gradDIS, 0.0));
    imageStore(framebufGradientPrevPix, gradPix, uvec4(curStrataPos));

}
*/