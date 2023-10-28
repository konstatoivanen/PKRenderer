#version 460
#extension GL_KHR_shader_subgroup_quad : require
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable

#pragma PROGRAM_COMPUTE

#include includes/GBuffers.glsl
#include includes/NoiseBlue.glsl
#include includes/Lighting.glsl
#include includes/Kernels.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_LightIndex)
{
    uint LightIndex;
};

layout(r8, set = PK_SET_DRAW) uniform image2D pk_Image;

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float depth = SampleViewDepth(coord);
    const float3 normal = SampleWorldNormal(coord);
    const float3 worldpos = SampleWorldPosition(coord, depth);

    const LightPacked light = Lights_LoadPacked(LightIndex);
    const half sourceAngle = half(0.5f * uintBitsToFloat(light.LIGHT_PACKED_SOURCERADIUS));

    // Only directional lights are supported here.
    const uint cascade = GetShadowCascadeIndex(depth);
    const uint index_shadow = (light.LIGHT_SHADOW) + cascade;
    const uint index_matrix = (light.LIGHT_MATRIX) + cascade;

    // Correct offsets by taking projection aspect into account
    const float4x4 lightMatrix = PK_BUFFER_DATA(pk_LightMatrices, index_matrix);
    const float clipWidth = length(lightMatrix[0].xyz);
    const float clipHeight = length(lightMatrix[1].xyz);
    const half aspect = half(clipWidth / clipHeight);

    const float3 posToLight = -light.LIGHT_POS;
    const float3 shadowPos = worldpos + Shadow_GetSamplingOffset(normal, posToLight) * (1.0f + cascade);
    
    const float4 uvw = GetLightClipUVW(shadowPos, index_matrix);
    const float z = uvw.z * light.LIGHT_RADIUS;

    half shadow = 0.0hf;

    // PCSS
    const half ditherAngle = half(Shadow_GradientNoise(pk_FrameRandom.y) * PK_TWO_PI);
    const half ditherScale = half(GlobalNoiseBlue(gl_GlobalInvocationID.xy, pk_FrameIndex.y).r);
    const half scale = fma(ditherScale, 0.3hf, 0.7hf) / half(cascade + 1u);
    const half maxOffset = 16.0hf * scale / half(SHADOW_SIZE.x);
    const half minOffset = (1.5hf / half(SHADOW_SIZE.x)) / maxOffset;

    const half sina = sin(ditherAngle) * maxOffset;
    const half cosa = cos(ditherAngle) * maxOffset;
    const half2x2 basis = half2x2(sina, cosa, -cosa * aspect, sina * aspect);

    half2 avgZ = 0.0hf.xx;

    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy);
        avgZ += Shadow_GatherMax(index_shadow, uvw.xy + offset, z);
    }

    const uint waveCount = subgroupAdd(uint(avgZ.y + 0.1hf));

    [[branch]]
    if (waveCount == 0u)
    {
        shadow = 1.0hf;
    }
    else if (waveCount == gl_SubgroupSize * 16)
    {
        shadow = 0.0hf;
    }
    else
    {
        avgZ.x /= avgZ.y;
        avgZ.x = clamp(sourceAngle * avgZ.x, minOffset, 1.0hf);

        for (uint i = 0u; i < 16u; ++i)
        {
            const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy) * avgZ.x;
            shadow += ShadowTest_PCF2x2(index_shadow, uvw.xy + offset, z);
        }

        shadow /= 16.0hf;
    }

    imageStore(pk_Image, coord, float4(shadow));
}