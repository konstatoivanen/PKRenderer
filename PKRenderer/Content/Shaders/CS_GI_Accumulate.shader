
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : require
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_multi_compile _ PK_GI_RESTIR
#pragma pk_program SHADER_STAGE_COMPUTE AccumulateCs

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include "includes/GBuffers.glsl"
#include "includes/SceneGI.glsl"
#include "includes/SceneGIReSTIR.glsl"

#define BOIL_FLT_GROUP_SIZE PK_W_ALIGNMENT_8
#define BOIL_FLT_MIN_LANE_COUNT 32

shared float lds_Weights[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];
shared uint lds_Count[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];

#define ReSTIR_Load_HitAsReservoir(coord, origin) ReSTIR_Unpack_Hit(GI_Load_Packed_Diff(coord), origin)

SHLuma ReSTIR_ResampleSpatioTemporal(const int2 coord_base, const int2 coord, const float depth, const float3 view_normal, const float3 origin, const Reservoir initial)
{
    const float depth_bias = lerp(0.1f, 0.01f, -view_normal.z);
    const float3 normal = ViewToWorldVec(view_normal);
    uint seed = ReSTIR_GetSeed(coord);
    int scale = 5;

    Reservoir combined = initial;

    // Temporal Resampling
    {
        const float2 fcoord_prev = WorldToClipUvPrev(origin) * int2(pk_ScreenSize.xy) + RESTIR_TEXEL_BIAS;
        const int2 coord_prev = GI_CollapseCheckerboardCoord(fcoord_prev, 1u);

        const uint hash = ReSTIR_Hash(seed++);
        int scale_bias = 0;

        for (uint i = 0u; i < RESTIR_SAMPLES_TEMPORAL; ++i)
        {
            scale = int(RESTIR_SAMPLES_TEMPORAL - i - 1);
            const int2 xy = ReSTIR_GetTemporalResamplingCoord(coord_prev, int(hash + i), scale, i == 0);
            const int2 xy_full = GI_ExpandCheckerboardCoord(xy, 1u);

            const float s_depth = PK_GI_SAMPLE_PREV_DEPTH(xy_full);
            const float3 s_normal = SamplePreviousViewNormal(xy_full);
            const float3 s_position = CoordToWorldPosPrev(xy_full, s_depth);

            [[branch]]
            if (Test_DepthReproject(depth, s_depth, depth_bias) && dot(view_normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                // Don't sample multiple temporal reservoirs to avoid boiling. Break on first accepted sample.
                Reservoir s_reservoir = ReSTIR_Load_Previous(xy);

                if (s_reservoir.M > 0u)
                {
                    ReSTIR_Normalize(s_reservoir, RESTIR_MAX_M);
                    ReSTIR_CombineReservoirSimple(combined, s_reservoir, hash);
                    break;
                }

            }
            
            // Sample vas invalidaded. reduce radius as this area is likely to have high frequency details.
            scale_bias--;
        }

        scale = clamp(scale + scale_bias, 2, 5);
    }

    // Spatial Resampling
    {
        [[loop]]
        for (uint i = 0u; i < RESTIR_SAMPLES_SPATIAL; ++i)
        {
            const uint hash = ReSTIR_Hash(seed + i);
            const int2 xy = ReSTIR_GetSpatialResamplingCoord(coord_base, scale, hash);
            const int2 xy_full = GI_ExpandCheckerboardCoord(xy);

            const float s_depth = PK_GI_SAMPLE_DEPTH(xy_full);
            const float3 s_normal = SampleWorldNormal(xy_full);
            const float3 s_position = CoordToWorldPos(xy_full, s_depth);
            const Reservoir s_reservoir = ReSTIR_Load_HitAsReservoir(xy, s_position);

            [[branch]]
            if (Any_NotEqual(xy, coord_base) && Test_DepthSurface(depth, s_depth, depth_bias) && dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                const float s_target_pdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_CombineReservoir(combined, s_reservoir, s_target_pdf, hash);
            }
        }

        seed += RESTIR_SAMPLES_SPATIAL;
    }

    // Subgroup Resampling
    {
        const uint hash = ReSTIR_Hash(seed);

        // Random sawp. tested quad swap but it produces more low freq noise.
        const uint wave_mask = gl_SubgroupSize - 1u;
        const uint shuffle_id = (gl_SubgroupInvocationID + (hash % wave_mask) + 1u) & wave_mask;

        Reservoir suffled;
        suffled.radiance = subgroupShuffle(combined.radiance, shuffle_id);
        suffled.position = subgroupShuffle(combined.position, shuffle_id);
        suffled.normal = subgroupShuffle(combined.normal, shuffle_id);
        suffled.target_pdf = subgroupShuffle(combined.target_pdf, shuffle_id);
        suffled.weight_sum = subgroupShuffle(combined.weight_sum, shuffle_id);
        suffled.M = subgroupShuffle(combined.M, shuffle_id);

        const float s_depth = subgroupShuffle(depth, shuffle_id);
        const float3 s_origin = subgroupShuffle(origin, shuffle_id);
        const float3 s_normal = subgroupShuffle(normal, shuffle_id);

        [[branch]]
        if (combined.M < RESTIR_MAX_M + 3 && Test_DepthSurface(depth, s_depth, depth_bias) && dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
        {
            const float s_target_pdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_origin, suffled);
            ReSTIR_CombineReservoir(combined, suffled, s_target_pdf, hash);
        }
    }

    // Boiling Filter
    bool is_boiling;
    {
        const float filter_strength = 0.2f;
        const float reservoir_weight = combined.target_pdf * combined.weight_sum;
        const float filter_multiplier = 10.f / clamp(filter_strength, 1e-6, 1.0) - 9.0f;

        const uint thread = gl_LocalInvocationID.x + gl_LocalInvocationID.y * BOIL_FLT_GROUP_SIZE;
        const uint wave = thread / gl_SubgroupSize;
        const uint4 thread_mask = subgroupBallot(reservoir_weight > 0.0f);

        uint thread_count = subgroupBallotBitCount(thread_mask);
        float wave_weight = subgroupAdd(reservoir_weight);

        if (subgroupElect())
        {
            lds_Weights[wave] = wave_weight;
            lds_Count[wave] = thread_count;
        }

        barrier();

        // Reduce the per-wavefront averages into a global average using one wavefront
        if (thread < (BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + gl_SubgroupSize - 1) / gl_SubgroupSize)
        {
            wave_weight = lds_Weights[thread];
            thread_count = lds_Count[thread];
            wave_weight = subgroupAdd(wave_weight);
            thread_count = subgroupAdd(thread_count);

            if (thread == 0)
            {
                lds_Weights[0] = lerp(0.0f, wave_weight / float(thread_count), thread_count > 0);
            }
        }

        barrier();

        is_boiling = reservoir_weight > lds_Weights[0] * filter_multiplier;
    }


    // Shade Hit & Store Temporal Reservoir

    // We should retrace visibility here as to not accumulate invalid samples.
    // However, this is expensive. Currently reservoirs are validated in async during present (which is virtually free).
    const bool reject = ReSTIR_NearFieldReject(depth, origin, initial, ReSTIR_Hash(seed + 1));
    const float3 combined_direction = normalize(combined.position - origin);
    const float diffuse_weight = dot(normal, combined_direction) * PK_INV_PI;
    const float weight = ReSTIR_GetSampleWeight(combined, normal, combined_direction) * diffuse_weight;
    const bool is_valid = !isnan(weight) && !isinf(weight) && weight > 0.0f && !reject;

    ReSTIR_Store_Current(coord_base, is_valid && !is_boiling ? combined : initial);

    const float3 radiance = lerp(initial.radiance, combined.radiance * weight, is_valid.xxx);
    const float3 direction = lerp(SafeNormalize(initial.position - origin), combined_direction, is_valid.xxx);
    return SH_Luma_FromRadiance(radiance, direction);
}

[numthreads(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 1u)]
void AccumulateCs()
{
    const int2 coord_base = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(coord_base));
    const float4 normal_roughness = SampleViewNormalRoughness(coord);
    const float depth = PK_GI_SAMPLE_DEPTH(coord);
    const bool is_scene = Test_DepthIsScene(depth);

    // Diffuse 
    {
        const float3 origin = CoordToWorldPos(coord, depth);
        const Reservoir reservoir = ReSTIR_Load_HitAsReservoir(coord_base, origin);
        const float4 sample_vector = NormalizeLength(reservoir.position - origin);

        GIDiff current;
        current.ao = min(1.0f, sample_vector.w / PK_GI_RAY_TMAX);

#if defined(PK_GI_RESTIR)
        current.sh = ReSTIR_ResampleSpatioTemporal(coord_base, coord, depth, normal_roughness.xyz, origin, reservoir);
#else
        current.sh = SH_Luma_FromRadiance(reservoir.radiance, sample_vector.xyz);
#endif

        GIDiff history = GI_Load_Diff(coord_base, PK_GI_STORE_LVL);
        const float alpha = GI_Alpha(history);

        float luma_max;
        GI_SUBGROUP_ANTIFIREFLY_MAXLUMA(is_scene, current, history, alpha, 1.0f, luma_max)
        current = GI_ClampLuma(current, luma_max);

        history = GI_Interpolate(history, current, GI_Alpha(history));
        GI_Store_Packed_Diff(coord_base, is_scene ? GI_Pack_Diff(history) : uint4(0));
    }

    // Specular
#if PK_GI_APPROX_ROUGH_SPEC == 1
    [[branch]]
    if (normal_roughness.w < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        [[branch]]
        if (subgroupAny(is_scene))
        {
            GISpec history = GI_Load_Spec(coord_base, PK_GI_STORE_LVL);
            GISpec current = GI_Load_Spec(coord_base);
            const float alpha = GI_Alpha(history);

            float luma_max;
            GI_SUBGROUP_ANTIFIREFLY_MAXLUMA(is_scene, current, history, alpha, (1.0f / (1e-4f + normal_roughness.w)), luma_max)
            current = GI_ClampLuma(current, luma_max);

            history = GI_Interpolate(history, current, alpha);
            GI_Store_Packed_Spec(coord_base, is_scene ? GI_Pack_Spec(history) : uint2(0));
        }
    }
}