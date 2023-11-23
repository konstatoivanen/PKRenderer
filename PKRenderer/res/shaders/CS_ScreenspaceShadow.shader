#extension GL_KHR_shader_subgroup_arithmetic : require

#multi_compile PASS_SHADOWMAP PASS_SHADOWMAP_UPSAMPLE PASS_SCREEN_DEPTH

#pragma PROGRAM_COMPUTE

#include includes/GBuffers.glsl
#include includes/NoiseBlue.glsl
#include includes/Lighting.glsl
#include includes/Kernels.glsl

layout(r8, set = PK_SET_DRAW) uniform image2D pk_Image;
layout(set = PK_SET_DRAW) uniform sampler2D pk_Texture;

#if defined(PASS_SHADOWMAP)

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    // This is only run on the first light.
    // Through sorting it should be a directional shadow casting light.
    // Appropriate checks are done on cpp side.
    // Note: Only directional lights are supported here!
    const uint LightIndex = 0u;

    const uint2 baseCoord = gl_GlobalInvocationID.xy;
    const int2 coord = int2(gl_GlobalInvocationID.xy * 2);

    // Quarter res needs a higher mip. biased depth is not build to hierarchical depth.
    // Resolve via gather4

    //@TODO this causes bad bias vectors in corners as they average towards view which can offset out of shadow unintentionally.
    // Perhaps solve this in the upsampling by eliminating outliers
    const float2 gatheruv = (coord + 1.0f) * pk_ScreenParams.zw;
    const float4 gatherGZ = GBUFFER_GATHER(pk_GB_Current_Normals, gatheruv, 2) - 0.5f;
    const float4 gatherGW = GBUFFER_GATHER(pk_GB_Current_Normals, gatheruv, 3) - 0.5f;

    // Normal downsampling causes viewdirection aligned vectors.
    // This yields bad bias offsets that pull corners out of shadow.
    // Fix this by clipping normal bias if the high res normals point to each other.
    const float2 n0 = float2(gatherGZ.x, gatherGW.x);
    const float2 n1 = float2(gatherGZ.y, gatherGW.y);
    const float2 n2 = float2(gatherGZ.z, gatherGW.z);
    const float2 n3 = float2(gatherGZ.w, gatherGW.w);
    const float4 normalDots = float4(dot(n0, n1), dot(n0, n2), dot(n3, n1), dot(n3, n2));
    const bool normalclip = Any_LEqual(normalDots, -1.0f.xxxx);

    const float4 gbuffernr = float4(0.0f.xx, dot(gatherGZ, 0.25f.xxxx) + 0.5f, dot(gatherGW, 0.25f.xxxx) + 0.5f);
    const float3 vnormal = DecodeGBufferViewNR(gbuffernr).xyz;
    const float3 normal = ViewToWorldVec(normalize(vnormal));

    const float4 depths = GatherViewDepthsBiased(gatheruv);
    const float  depth = cmin(depths);

    const LightPacked light = Lights_LoadPacked(LightIndex);
    const float3 posToLight = -light.LIGHT_POS;

    const half ditherAngle = half(Shadow_GradientNoise(float2(baseCoord), pk_FrameIndex.y) * PK_TWO_PI);
    const half ditherScale = half(Shadow_GradientNoise(float2(baseCoord), pk_FrameIndex.y + 1u));

    const float sourceAngle = uintBitsToFloat(light.LIGHT_PACKED_SOURCERADIUS);
    const half maxRadius = half(sourceAngle * 2.0f);
    const half sinAlpha = half(sourceAngle * 0.5f);

    // Correct offsets by taking projection size into account
    const uint cascade = GetShadowCascadeIndex(depth);
    const float4x4 lightMatrix = PK_BUFFER_DATA(pk_LightMatrices, cascade);
    const half scalex = half(length(lightMatrix[0].xyz)) * maxRadius;
    const half scaley = half(length(lightMatrix[1].xyz)) * maxRadius;
    const half2 scale = half2(scalex, scaley) * fma(ditherScale, 0.3hf, 0.7hf);

    const half2 minOffset = half(1.0f / SHADOW_SIZE.x) / scale;
    const half cosa = cos(ditherAngle);
    const half sina = sin(ditherAngle);
    const half2x2 basis = half2x2(sina * scale.x, cosa * scale.x, -cosa * scale.y, sina * scale.y);

    float3 worldpos = CoordToWorldPos(coord, depth);
    const float2 biasFactors = Shadow_GetBiasFactors(normal, posToLight);
    worldpos += biasFactors.x * normal * SHADOW_NEAR_BIAS * float(normalclip) * (1.0f + cascade);
    worldpos += biasFactors.y * posToLight * SHADOW_NEAR_BIAS * (1.0f + cascade) * (1.0f + 0.1f / sqrt(depth));
    
    const float2 uv = LightClipToUV(lightMatrix * float4(worldpos, 1.0f)).xy;
    const float z = dot(light.LIGHT_POS, worldpos) + light.LIGHT_RADIUS;

    // PCSS 
    half2 avgZ = 0.0hf.xx;

    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy);
        avgZ += Shadow_GatherMax(cascade, uv + offset, z);
    }

    const uint validSamples = uint(avgZ.y);

    half shadow = 0.0hf;

    [[branch]]
    if (subgroupAll(validSamples == 0u))
    {
        shadow = 1.0hf;
    }
    else [[branch]] if (subgroupAll(validSamples == 16u))
    {
        shadow = 0.0hf;
    }
    else
    {
        avgZ.x /= avgZ.y;
        half2 penumbra = clamp(sinAlpha * avgZ.xx, minOffset, 1.0hf.xx);

        for (uint i = 0u; i < 16u; ++i)
        {
            const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy) * penumbra;
            shadow += ShadowTest_PCF2x2(cascade, uv + offset, z);
        }

        shadow /= 16.0hf;
    }

    imageStore(pk_Image, int2(baseCoord), float4(shadow));
}

#elif defined(PASS_SHADOWMAP_UPSAMPLE)

#define GROUP_SIZE 8
shared half lds_shadow[GROUP_SIZE * GROUP_SIZE];
shared float lds_depth[GROUP_SIZE * GROUP_SIZE];

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const int thread = int(gl_LocalInvocationIndex);
    const int2 baseCoord = int2(gl_WorkGroupID.xy) * GROUP_SIZE - int2(4);
    const int2 loadCoord = baseCoord + int2(thread % GROUP_SIZE, thread / GROUP_SIZE) * 2;
    
    const float4 baseDepths = GatherViewDepths((loadCoord + 1.0f.xx) * pk_ScreenParams.zw);
    const bool farclip = Any_GEqual(baseDepths, pk_ClipParams.yyyy - 1e-2f);
    const float baseDepth = lerp(cmin(baseDepths), -pk_ClipParams.y * 2.0f, farclip);
    lds_depth[thread] = baseDepth;

    const half2 offset = half2(GlobalNoiseBlue(gl_GlobalInvocationID.xy, pk_FrameIndex.y).xy) * 2.0hf - 1.0hf;
    const half2 baseuv = half2(gl_LocalInvocationID.xy + 0.5f) * 0.5hf + 1.5hf.xx + offset;
    
    half2 offsets[4] =
    {
        half2(-1.0hf, -1.0hf),
        half2(-1.0hf, +1.0hf),
        half2(+1.0hf, -1.0hf),
        half2(+1.0hf, +1.0hf)
    };

    byte4 indices[4];
    half4 weights[4];

    lds_shadow[thread] = half(texelFetch(pk_Texture, loadCoord / 2, 0).x);

    [[unroll]]
    for (uint i = 0u; i < 4; ++i)
    {
        // Filter radius can clip into lds bounds. Smaller filter is less effective.
        // Clamp offset instead.
        const half2 sampleUV = clamp(baseuv + offsets[i], 0.0hf, 6.999hf);
        const byte2 sampleCoord = byte2(sampleUV);

        byte4 s_indices = sampleCoord.yyyy;
        s_indices += byte4(0u, 0u, 1u, 1u);
        s_indices *= byte(8u);
        s_indices += sampleCoord.xxxx;
        s_indices += byte4(0, 1, 0, 1);

        half2 f = fract(sampleUV);
        half4 fw = half4(f.xy, 1.0hf - f.xy);
        weights[i] = fw.zxzx * fw.wwyy;
        indices[i] = s_indices;
    }

    barrier();

    const float depth = SampleViewDepth(int2(gl_GlobalInvocationID.xy));
    const float k_depth = 20.0f / sqrt(depth);

    half shadow = 0.0hf;
    half wsum = 0.0hf;

    [[unroll]]
    for (uint i = 0u; i < 4; ++i)
    {
        byte4 s_indices = indices[i];
        half4 s_weights = weights[i];
        
        float4 depths;
        depths.x = lds_depth[s_indices.x];
        depths.y = lds_depth[s_indices.y];
        depths.z = lds_depth[s_indices.z];
        depths.w = lds_depth[s_indices.w];

        s_weights *= half4(exp(-abs(depths - depth.xxxx) * k_depth));
        
        shadow = fma(lds_shadow[s_indices.x], s_weights.x, shadow);
        shadow = fma(lds_shadow[s_indices.y], s_weights.y, shadow);
        shadow = fma(lds_shadow[s_indices.z], s_weights.z, shadow);
        shadow = fma(lds_shadow[s_indices.w], s_weights.w, shadow);
        wsum += dot(s_weights, 1.0hf.xxxx);
    }
    
    shadow /= wsum;

    imageStore(pk_Image, int2(gl_GlobalInvocationID.xy), float4(shadow));
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

float BEND_SAMPLE_DEPTH(float2 uv) { return SampleClipDepthBiased(int2(uv * pk_ScreenSize.xy)); }

#include includes/bend_sss_gpu.glsl

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
