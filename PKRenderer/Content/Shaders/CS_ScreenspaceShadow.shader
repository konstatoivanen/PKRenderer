
#extension GL_KHR_shader_subgroup_arithmetic : require
#pragma pk_multi_compile PASS_SHADOWMAP PASS_SHADOWMAP_UPSAMPLE PASS_SCREEN_DEPTH
#pragma pk_program SHADER_STAGE_COMPUTE ShadowmapCs PASS_SHADOWMAP
#pragma pk_program SHADER_STAGE_COMPUTE ShadowmapUpsampleCs PASS_SHADOWMAP_UPSAMPLE
#pragma pk_program SHADER_STAGE_COMPUTE ScreenSpaceShadowsCs PASS_SCREEN_DEPTH

#include "includes/GBuffers.glsl"
#include "includes/NoiseBlue.glsl"
#include "includes/LightSampling.glsl"
#include "includes/Kernels.glsl"

uniform image2D pk_Image;
uniform sampler2D pk_Texture;

[pk_numthreads(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 1u)]
void ShadowmapCs()
{
    // This is only run on the first light.
    // Through sorting it should be a directional shadow casting light.
    // Appropriate checks are done on cpp side.
    // Note: Only directional lights are supported here!
    const uint light_index = 0u;

    const int2 coord_base = int2(gl_GlobalInvocationID.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy * 2);

    // Quarter res needs a higher mip. biased depth is not build to hierarchical depth.
    // Resolve via gather4

    //@TODO this causes bad bias vectors in corners as they average towards view which can offset out of shadow unintentionally.
    // Perhaps solve this in the upsampling by eliminating outliers
    const float2 uv_gather = (coord + 1.0f) * pk_ScreenParams.zw;
    const float4 gz_gather = GBUFFER_GATHER(pk_GB_Current_Normals, uv_gather, 2) - 0.5f;
    const float4 gw_gather = GBUFFER_GATHER(pk_GB_Current_Normals, uv_gather, 3) - 0.5f;

    // Normal downsampling causes viewdirection aligned vectors.
    // This yields bad bias offsets that pull corners out of shadow.
    // Fix this by clipping normal bias if the high res normals point to each other.
    const float2 n0 = float2(gz_gather.x, gw_gather.x);
    const float2 n1 = float2(gz_gather.y, gw_gather.y);
    const float2 n2 = float2(gz_gather.z, gw_gather.z);
    const float2 n3 = float2(gz_gather.w, gw_gather.w);
    const float4 normal_dots = float4(dot(n0, n1), dot(n0, n2), dot(n3, n1), dot(n3, n2));
    const bool normal_clip = Any_LEqual(normal_dots, -1.0f.xxxx);

    const float4 gbuffer_normal_roughness = float4(0.0f.xx, dot(gz_gather, 0.25f.xxxx) + 0.5f, dot(gw_gather, 0.25f.xxxx) + 0.5f);
    const float3 view_normal = DecodeGBufferViewNR(gbuffer_normal_roughness).xyz;
    const float3 normal = ViewToWorldVec(normalize(view_normal));

    const float4 depths = GatherViewDepthsBiased(uv_gather);
    const float  depth = cmin(depths);

    const SceneLight light = Lights_LoadLight(light_index);

    const half dither_angle = half(InterleavedGradientNoise(float2(coord_base), pk_FrameIndex.y) * PK_TWO_PI);
    const half dither_scale = half(InterleavedGradientNoise(float2(coord_base), pk_FrameIndex.y + 1u));

    const float source_angle = light.source_radius;
    const half max_radius = half(source_angle * 2.0f);
    const half sin_alpha = half(source_angle * 0.5f);

    // Correct offsets by taking projection size into account
    const uint cascade = GetShadowCascadeIndex(depth);
    const float4x4 light_matrix = pk_LightMatrices[cascade + 1u];
    const half scale_x = half(length(light_matrix[0].xyz)) * max_radius;
    const half scale_y = half(length(light_matrix[1].xyz)) * max_radius;
    const half2 scale = half2(scale_x, scale_y) * fma(dither_scale, 0.3hf, 0.7hf);

    const half2 offset_min = half(1.0f / SHADOW_SIZE.x) / scale;
    const half cosa = cos(dither_angle);
    const half sina = sin(dither_angle);
    const half2x2 basis = half2x2(sina * scale.x, cosa * scale.x, -cosa * scale.y, sina * scale.y);

    float3 world_pos = UvToWorldPos(uv_gather, depth);
    const float2 bias_factors = Shadow_GetBiasFactors(normal, -light.position);
    world_pos += bias_factors.x * normal * SHADOW_NEAR_BIAS * float(normal_clip) * (1.0f + cascade * cascade);
    world_pos += bias_factors.y * -light.position * SHADOW_NEAR_BIAS * (1.0f + cascade) * (1.0f + 0.1f / sqrt(depth));

    const float2 uv = ClipToUv((light_matrix * float4(world_pos, 1.0f)).xyw);
    const float z = dot(light.position, world_pos) + light.radius;

    // PCSS 
    half2 average_z = 0.0hf.xx;

    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy);
        average_z += Shadow_GatherMax(cascade, uv + offset, z);
    }

    const uint validSamples = uint(average_z.y);

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
        average_z.x /= average_z.y;
        half2 penumbra = clamp(sin_alpha * average_z.xx, offset_min, 1.0hf.xx);

        for (uint i = 0u; i < 16u; ++i)
        {
            const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy) * penumbra;
            shadow += ShadowTest_PCF2x2(cascade, uv + offset, z);
        }

        shadow /= 16.0hf;
    }

    imageStore(pk_Image, coord_base, float4(shadow));
}

#define GROUP_SIZE 8

[pk_local(ShadowmapUpsampleCs)] shared half lds_shadow[GROUP_SIZE * GROUP_SIZE];
[pk_local(ShadowmapUpsampleCs)] shared float lds_depth[GROUP_SIZE * GROUP_SIZE];

[pk_numthreads(GROUP_SIZE, GROUP_SIZE, 1u)]
void ShadowmapUpsampleCs()
{
    const int thread = int(gl_LocalInvocationIndex);
    const int2 coord_base = int2(gl_WorkGroupID.xy) * GROUP_SIZE - int2(4);
    const int2 coord_load = coord_base + int2(thread % GROUP_SIZE, thread / GROUP_SIZE) * 2;

    const float4 base_depths = GatherViewDepths((coord_load + 1.0f.xx) * pk_ScreenParams.zw);
    const bool far_clip = Any_GEqual(base_depths, pk_ClipParams.yyyy - 1e-2f);
    const float base_depth = lerp(cmin(base_depths), -pk_ClipParams.y * 2.0f, far_clip);
    lds_depth[thread] = base_depth;

    const half2 offset = half2(GlobalNoiseBlue(gl_GlobalInvocationID.xy, pk_FrameIndex.y).xy) * 2.0hf - 1.0hf;
    const half2 uv_base = half2(gl_LocalInvocationID.xy + 0.5f) * 0.5hf + 1.5hf.xx + offset;

    half2 offsets[4] =
    {
        half2(-1.0hf, -1.0hf),
        half2(-1.0hf, +1.0hf),
        half2(+1.0hf, -1.0hf),
        half2(+1.0hf, +1.0hf)
    };

    byte4 indices[4];
    half4 weights[4];

    lds_shadow[thread] = half(texelFetch(pk_Texture, coord_load / 2, 0).x);

    [[unroll]]
    for (uint i = 0u; i < 4; ++i)
    {
        // Filter radius can clip into lds bounds. Smaller filter is less effective.
        // Clamp offset instead.
        const half2 s_uv = clamp(uv_base + offsets[i], 0.0hf, 6.999hf);
        const byte2 s_coord = byte2(s_uv);

        byte4 s_indices = s_coord.yyyy;
        s_indices += byte4(0u, 0u, 1u, 1u);
        s_indices *= byte(8u);
        s_indices += s_coord.xxxx;
        s_indices += byte4(0, 1, 0, 1);

        half2 f = fract(s_uv);
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

#if defined(PASS_SCREEN_DEPTH)
    #define WAVE_SIZE 64            // Wavefront size of the compute shader running this code. 
    #define SAMPLE_COUNT 60         // Number of shadow samples per-pixel.
    #define HARD_SHADOW_SAMPLES 4   // Number of initial shadow samples that will produce a hard shadow, and not perform sample-averaging.
    #define FADE_OUT_SAMPLES 8      // Number of samples that will fade out at the end of the shadow (for a minor cost).
    float BEND_SAMPLE_DEPTH(float2 uv) { return SampleClipDepthBiased(int2(uv * pk_ScreenSize.xy)); }
    #include "includes/bend_sss_gpu.glsl"
#endif

[pk_local(ScreenSpaceShadowsCs)] uniform float4 pk_LightCoordinate;
[pk_local(ScreenSpaceShadowsCs)] uniform int2 pk_WaveOffset;

[pk_numthreads(WAVE_SIZE, 1u, 1u)]
void ScreenSpaceShadowsCs()
{
    DispatchParameters dispatch_params;
    dispatch_params.LightCoordinate = pk_LightCoordinate;	    // Values stored in DispatchList::LightCoordinate_Shader by BuildDispatchList()
    dispatch_params.WaveOffset = pk_WaveOffset;					// Values stored in DispatchData::WaveOffset_Shader by BuildDispatchList()
    dispatch_params.FarDepthValue = 0.0f;				        // Set to the Depth Buffer Value for the far clip plane, as determined by renderer projection matrix setup (typically 0).
    dispatch_params.NearDepthValue = 1.0f;				        // Set to the Depth Buffer Value for the near clip plane, as determined by renderer projection matrix setup (typically 1).
    dispatch_params.InvDepthTextureSize = pk_ScreenParams.zw;	// Inverse of the texture dimensions for 'DepthTexture' (used to convert from pixel coordinates to UVs)
    dispatch_params.SurfaceThickness = 0.01;
    dispatch_params.BilinearThreshold = 0.04;
    dispatch_params.ShadowContrast = 4;
    dispatch_params.IgnoreEdgePixels = false;
    dispatch_params.UsePrecisionOffset = false;
    dispatch_params.BilinearSamplingOffsetMode = false;
    dispatch_params.DebugOutputEdgeMask = false;
    dispatch_params.DebugOutputThreadIndex = false;
    dispatch_params.DebugOutputWaveIndex = false;
    dispatch_params.DepthBounds = float2(0, 1);
    dispatch_params.UseEarlyOut = true;
    WriteScreenSpaceShadow(dispatch_params, int3(gl_WorkGroupID), int(gl_LocalInvocationIndex));
}
