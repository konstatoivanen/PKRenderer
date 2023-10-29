#version 460
#extension GL_KHR_shader_subgroup_vote : require

#multi_compile PASS_SHADOWMAP PASS_SCREEN_DEPTH

#pragma PROGRAM_COMPUTE

#include includes/GBuffers.glsl

layout(r8, set = PK_SET_DRAW) uniform image2D pk_Image;

#if defined(PASS_SHADOWMAP)

#include includes/NoiseBlue.glsl
#include includes/Lighting.glsl
#include includes/Kernels.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_LightIndex)
{
    uint LightIndex;
};

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

    for (uint i = 0u; i < 8u; ++i)
    {
        const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i * 2].xy);
        avgZ += Shadow_GatherMax(index_shadow, uvw.xy + offset, z);
    }

    const uint validSamples = uint(avgZ.y + 0.1hf);

    [[branch]]
    if (subgroupAll(validSamples == 0u))
    {
        shadow = 1.0hf;
    }
    else if (subgroupAll(validSamples == 8u))
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

#else

// Wavefront size of the compute shader running this code. 
#define WAVE_SIZE 64
// Number of shadow samples per-pixel.
#define SAMPLE_COUNT 60
// Number of initial shadow samples that will produce a hard shadow, and not perform sample-averaging.
#define HARD_SHADOW_SAMPLES 4
// Number of samples that will fade out at the end of the shadow (for a minor cost).
#define FADE_OUT_SAMPLES 8

float BEND_SAMPLE_DEPTH(float2 uv) 
{
    float depth = texelFetch(pk_GB_Current_Depth, int2(uv * pk_ScreenSize.xy), 0).x;
    float bias = texelFetch(pk_GB_Current_ZBias, int2(uv * pk_ScreenSize.xy), 0).x;
    return bias + depth;
}

#include includes/ScreenSpaceShadow.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_BendShadowDispatchData)
{
    float4 LightCoordinate;
    int2 WaveOffset;
};

layout(local_size_x = WAVE_SIZE, local_size_y = 1, local_size_z = 1) in;
void main()
{
    DispatchParameters dispatchParams;

    dispatchParams.LightCoordinate = LightCoordinate;				// Values stored in DispatchList::LightCoordinate_Shader by BuildDispatchList()
    dispatchParams.WaveOffset = WaveOffset;					// Values stored in DispatchData::WaveOffset_Shader by BuildDispatchList()
    dispatchParams.FarDepthValue = 0.0f;				// Set to the Depth Buffer Value for the far clip plane, as determined by renderer projection matrix setup (typically 0).
    dispatchParams.NearDepthValue = 1.0f;				// Set to the Depth Buffer Value for the near clip plane, as determined by renderer projection matrix setup (typically 1).
    dispatchParams.InvDepthTextureSize = pk_ScreenParams.zw;			// Inverse of the texture dimensions for 'DepthTexture' (used to convert from pixel coordinates to UVs)
    dispatchParams.SurfaceThickness = 0.01;
    dispatchParams.BilinearThreshold = 0.04;
    dispatchParams.ShadowContrast = 4;
    dispatchParams.IgnoreEdgePixels = false;
    dispatchParams.UsePrecisionOffset = false;
    dispatchParams.BilinearSamplingOffsetMode = false;
    dispatchParams.DebugOutputEdgeMask = false;
    dispatchParams.DebugOutputThreadIndex = false;
    dispatchParams.DebugOutputWaveIndex = false;
    dispatchParams.DepthBounds = float2(0, 1);
    dispatchParams.UseEarlyOut = false;

    float shadow = 0.0f;
    int2 coord = int2(0);
    WriteScreenSpaceShadow(dispatchParams, int3(gl_WorkGroupID), int(gl_LocalInvocationIndex), shadow, coord);

    shadow = min(shadow, imageLoad(pk_Image, coord).x);

    imageStore(pk_Image, coord, float4(shadow));
}

#endif
