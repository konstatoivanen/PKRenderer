#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

layout(rgba16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_SHY_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_CoCg_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2D pk_ScreenGI_Hits;
layout(r8ui, set = PK_SET_SHADER) uniform readonly restrict uimage2D pk_ScreenGI_Mask;

float3 SampleEnvironment(float3 direction, float roughness)
{
    return HDRDecode(tex2DLod(pk_SceneOEM_HDR, OctaUV(direction), 4.0f * roughness)).rgb * pk_SceneOEM_Exposure;
}

float3 SampleRadiance(const float3 origin, const float3 direction, const float dist, const float roughness)
{
    const float level = roughness * roughness * sqrt(dist / PK_GI_VOXEL_SIZE);
    const float4 voxel = SampleGI_WS(origin + direction * dist, level);

    const float3 env = SampleEnvironment(direction, 0.25f);
    const float envclip = saturate(PK_GI_RAY_MAX_DISTANCE * (1.0f - (dist / PK_GI_RAY_MAX_DISTANCE)));
    const float alpha = max(voxel.a, 1.0f / PK_GI_VOXEL_MAX_MIP);

    return lerp(env, voxel.rgb / alpha, envclip);
}

SH SampleIrradianceSH(const float3 O, const float3 N, int2 coord)
{
    const float3 dither = GlobalNoiseBlue(uint2(coord) + pk_FrameIndex.xx).xyz;
    const float3 direction = ImportanceSampleGGX(pk_SceneGI_SampleIndex, pk_SceneGI_SampleCount, N, 1.0f, dither.xy);
    const float sampleDistance = imageLoad(pk_ScreenGI_Hits, coord).r;
    const float3 radiance = SampleRadiance(O, direction, sampleDistance, 0.0f);
    return IrradianceToSH(radiance, direction);
}

SH SampleRadianceSH(const float3 O, const float3 N, const float3 V, int2 coord, float roughness)
{
    const float3 dither = GlobalNoiseBlue(uint2(coord)+pk_FrameIndex.xx).xyz;
    const float3 direction = ImportanceSampleGGX(pk_SceneGI_SampleIndex, pk_SceneGI_SampleCount, N, V, roughness, dither.xy);
    const float sampleDistance = imageLoad(pk_ScreenGI_Hits, coord).g;
    const float3 radiance = SampleRadiance(O, direction, sampleDistance, roughness);
    return IrradianceToSH(radiance, direction);
}

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    int2 size = imageSize(pk_ScreenGI_Mask).xy;
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    uint mask = imageLoad(pk_ScreenGI_Mask, coord).r;
    bool hasDiscontinuity = (mask & (1 << 0)) != 0;
    bool isActive = (mask & (1 << 1)) != 0;
    bool isOOB = (mask & (1 << 2)) != 0;

    if (isOOB)
    {
        return;
    }
    
    // Find a base for the side cones with the normal as one of its base vectors.
    const float4 NR = SampleWorldSpaceNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3 O = SampleWorldPosition(coord, size);
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
    const float3 D = GlobalNoiseBlue(uint2(coord)).xyz;
    const float2 UV = (coord + 0.5f.xx) / size;

    SH irradianceSH;
    irradianceSH.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(UV, PK_GI_DIFF_LVL)).rgba;
    irradianceSH.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(UV, PK_GI_DIFF_LVL)).rg;
    
    SH radianceSH;
    radianceSH.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(UV, PK_GI_SPEC_LVL)).rgba;
    radianceSH.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(UV, PK_GI_SPEC_LVL)).rg;

    const bool refreshDiff = IsNaN(irradianceSH.SHY.w) || IsNaN(irradianceSH.CoCg);
    const bool refreshSpec = IsNaN(radianceSH.SHY.w) || IsNaN(radianceSH.CoCg);
    const float interDiff = hasDiscontinuity ? 0.5f : 0.05f;
    const float interSpec = hasDiscontinuity ? 0.5f : lerp(0.05f, 0.25f, NR.w);

    SH irradianceNew = SampleIrradianceSH(O, N, coord);
    SH radianceNew = SampleRadianceSH(O, N, V, coord, NR.w);

    irradianceSH = refreshDiff ? irradianceNew : InterpolateSH(irradianceSH, irradianceNew, interDiff);
    radianceSH = refreshSpec ? radianceNew : InterpolateSH(radianceSH, radianceNew, interSpec);
    
    imageStore(pk_ScreenGI_SHY_Write, int3(coord, PK_GI_DIFF_LVL), irradianceSH.SHY);
    imageStore(pk_ScreenGI_CoCg_Write, int3(coord, PK_GI_DIFF_LVL), float4(irradianceSH.CoCg, 0.0f.xx));

    imageStore(pk_ScreenGI_SHY_Write, int3(coord, PK_GI_SPEC_LVL), radianceSH.SHY);
    imageStore(pk_ScreenGI_CoCg_Write, int3(coord, PK_GI_SPEC_LVL), float4(radianceSH.CoCg, 0.0f.xx));
}