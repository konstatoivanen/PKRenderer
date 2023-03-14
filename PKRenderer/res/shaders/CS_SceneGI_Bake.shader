#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl
#include includes/SHL1.glsl

layout(rgba16f, set = PK_SET_SHADER) uniform image2D pk_ScreenGI_SHY;
layout(rg16f, set = PK_SET_SHADER) uniform image2D pk_ScreenGI_CoCg;
layout(r8ui, set = PK_SET_SHADER) uniform readonly restrict uimage2D pk_ScreenGI_Mask;

float4 IntegrateSH(const float3 origin, const float3 normal, int2 coord, int2 size)
{
    SH currentSH;
    currentSH.SHY = imageLoad(pk_ScreenGI_SHY, coord).rgba;
    currentSH.CoCg = imageLoad(pk_ScreenGI_CoCg, coord).rg;

    const float3 dither = GlobalNoiseBlue(uint2(coord) + pk_FrameIndex.xx).xyz;
    const float3 direction = GetSampleDirectionHammersLey(pk_SceneGI_SampleIndex, pk_SceneGI_SampleCount, dither.xy, normal);
    const float sampleDistance = imageLoad(pk_ScreenGI_Hits, coord).r;

    const float level = sqrt(sampleDistance / PK_GI_VOXEL_SIZE);
    const float4 voxel = SampleSceneGI(origin + direction * sampleDistance, level);

    const float3 environment = HDRDecode(tex2DLod(pk_SceneOEM_HDR, OctaUV(direction), 4.0f)) * pk_SceneOEM_Exposure;
    const float environmentFade = saturate(PK_GI_RAY_MAX_DISTANCE * (1.0f - (sampleDistance / PK_GI_RAY_MAX_DISTANCE)));

    const float3 irradiance = lerp(environment, voxel.rgb / (voxel.a + 1.0e-4f), environmentFade);
    const SH irradianceSH = IrradianceToSH(irradiance, direction);

    AccumulateSH(currentSH, irradianceSH, pk_SceneGI_SampleCount);

    imageStore(pk_ScreenGI_SHY, coord, currentSH.SHY);
    imageStore(pk_ScreenGI_CoCg, coord, float4(currentSH.CoCg, 0.0f.xx));

    return float4(SHToIrradiance(currentSH, normal), 0.0f);
}


layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
    int2 size = imageSize(pk_ScreenGI_Write).xy;
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    uint mask = imageLoad(pk_ScreenGI_Mask, coord).r;
    bool hasDiscontinuity = (mask & (1 << 0)) != 0;
    bool isActive = (mask & (1 << 1)) != 0;
    bool isOOB = (mask & (1 << 2)) != 0;

    float3 worldposition = SampleWorldPosition(coord, size);

    if (!isOOB)
    {
        // Find a base for the side cones with the normal as one of its base vectors.
        const float4 NR = SampleWorldSpaceNormalRoughness(coord);
        const float3 N = NR.xyz;
        const float3 O = worldposition;
        const float3 V = normalize(worldposition - pk_WorldSpaceCameraPos.xyz);
        const float3 R = reflect(V, N);
        const float3 D = GlobalNoiseBlue(uint2(coord)).xyz;

        //ConeTraceDiffuse(O, N, D.x));
        //GatherRayHits(coord, O, N, D.xy);
        imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_DIFF_LVL), IntegrateSH(O, N, coord, size));

        if (hasDiscontinuity || isActive)
        {
            imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_SPEC_LVL), ConeTraceSpecular(O, N, R, NR.w));
        }
    }
}