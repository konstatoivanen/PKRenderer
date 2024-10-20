
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : require
#pragma pk_multi_compile _ PK_GI_CHECKERBOARD_TRACE
#pragma pk_multi_compile _ PK_GI_RESTIR
#pragma pk_program SHADER_STAGE_COMPUTE main

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include "includes/GBuffers.glsl"
#include "includes/SceneGI.glsl"
#include "includes/SceneGIReSTIR.glsl"

#define BOIL_FLT_GROUP_SIZE PK_W_ALIGNMENT_8
#define BOIL_FLT_MIN_LANE_COUNT 32
shared float s_weights[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];
shared uint s_count[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];

// A novel anti-firefly filter using subgroup intrisics.
// This is a lot cheaper than the classics 3x3 filter but a bit less effective due to larger sample area.
#define SUBGROUP_ANTIFIREFLY_FILTER(condition, current, history, alpha, scale)                  \
{                                                                                               \
    const uint4 threadMask = subgroupBallot(condition);                                         \
    const uint threadCount = max(1u, subgroupBallotBitCount(threadMask)) - 1u;                  \
                                                                                                \
    const float2 moments = make_moments(GI_Luminance(current));                                 \
    const float2 momentsWave = (subgroupAdd(moments) - moments) / threadCount;                  \
                                                                                                \
    const float variance = pow(abs(momentsWave.y - pow2(momentsWave.x)), 0.25f);                \
    const float lumaMax = lerp(GI_Luminance(history), momentsWave.x, alpha) + variance * scale; \
                                                                                                \
    current = GI_ClampLuma(current, lumaMax);                                                   \
}                                                                                               \

#define ReSTIR_Load_HitAsReservoir(coord, origin) ReSTIR_Unpack_Hit(GI_Load_Packed_Diff(coord), origin)

SH ReSTIR_ResampleSpatioTemporal(const int2 baseCoord, const int2 coord, const float depth, const float3 viewnormal, const float3 origin, const Reservoir initial)
{
    const float depthBias = lerp(0.1f, 0.01f, -viewnormal.z);
    const float3 normal = ViewToWorldVec(viewnormal);
    uint seed = ReSTIR_GetSeed(coord);
    int scale = 5;

    Reservoir combined = initial;

    // Temporal Resampling
    {
        const float2 screenUvPrev = WorldToClipUVPrev(origin) * int2(pk_ScreenSize.xy) + RESTIR_TEXEL_BIAS;
        const int2 coordPrev = GI_CollapseCheckerboardCoord(screenUvPrev, 1u);
        const uint hash = ReSTIR_Hash(seed++);
        int scaleBias = 0;

        for (uint i = 0u; i < RESTIR_SAMPLES_TEMPORAL; ++i)
        {
            scale = int(RESTIR_SAMPLES_TEMPORAL - i - 1);
            const int2 xy = ReSTIR_GetTemporalResamplingCoord(coordPrev, int(hash + i), scale, i == 0);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy, 1u);
            const float s_depth = PK_GI_SAMPLE_PREV_DEPTH(xyFull);
            const float3 s_normal = SamplePreviousViewNormal(xyFull);
            const float3 s_position = CoordToWorldPosPrev(xyFull, s_depth);

            [[branch]]
            if (Test_DepthReproject(depth, s_depth, depthBias) && dot(viewnormal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                // Don't sample multiple temporal reservoirs to avoid boiling. Break on first accepted sample.
                Reservoir s_reservoir = ReSTIR_Load_Previous(xy);

                if (s_reservoir.M > 0u)
                {
                    ReSTIR_Normalize(s_reservoir, RESTIR_MAX_M);
                    ReSTIR_CombineReservoirSimple(combined, s_reservoir, hash);
                    break;
                }

                // Sample vas invalidaded. reduce radius as this area is likely to have high frequency details.
                scaleBias--;
            }
        }

        scale = clamp(scale + scaleBias, 2, 5);
    }

    // Spatial Resampling
    {
        [[loop]]
        for (uint i = 0u; i < RESTIR_SAMPLES_SPATIAL; ++i)
        {
            const uint hash = ReSTIR_Hash(seed + i);
            const int2 xy = ReSTIR_GetSpatialResamplingCoord(baseCoord, scale, hash);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);
            const float s_depth = PK_GI_SAMPLE_DEPTH(xyFull);
            const float3 s_normal = SampleWorldNormal(xyFull);
            const float3 s_position = CoordToWorldPos(xyFull, s_depth);
            const Reservoir s_reservoir = ReSTIR_Load_HitAsReservoir(xy, s_position);

            [[branch]]
            if (Any_NotEqual(xy, baseCoord) && Test_DepthSurface(depth, s_depth, depthBias) && dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                const float s_targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_CombineReservoir(combined, s_reservoir, s_targetPdf, hash);
            }
        }

        seed += RESTIR_SAMPLES_SPATIAL;
    }

    // Subgroup Resampling
    {
        const uint hash = ReSTIR_Hash(seed);

        // Random sawp. tested quad swap but it produces more low freq noise.
        const uint subgroupMask = gl_SubgroupSize - 1u;
        const uint shuffleId = (gl_SubgroupInvocationID + (hash % subgroupMask) + 1u) & subgroupMask;

        Reservoir suffled;
        suffled.radiance = subgroupShuffle(combined.radiance, shuffleId);
        suffled.position = subgroupShuffle(combined.position, shuffleId);
        suffled.normal = subgroupShuffle(combined.normal, shuffleId);
        suffled.targetPdf = subgroupShuffle(combined.targetPdf, shuffleId);
        suffled.weightSum = subgroupShuffle(combined.weightSum, shuffleId);
        suffled.M = subgroupShuffle(combined.M, shuffleId);

        const float s_depth = subgroupShuffle(depth, shuffleId);
        const float3 s_origin = subgroupShuffle(origin, shuffleId);
        const float3 s_normal = subgroupShuffle(normal, shuffleId);

        [[branch]]
        if (combined.M < RESTIR_MAX_M + 3 && Test_DepthSurface(depth, s_depth, depthBias) && dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
        {
            const float s_targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_origin, suffled);
            ReSTIR_CombineReservoir(combined, suffled, s_targetPdf, hash);
        }
    }

    // Boiling Filter
    bool isBoiling;
    {
        const float filterStrength = 0.2f;
        const float reservoirWeight = combined.targetPdf * combined.weightSum;
        const float boilingFilterMultiplier = 10.f / clamp(filterStrength, 1e-6, 1.0) - 9.0f;

        const uint linearThreadIndex = gl_LocalInvocationID.x + gl_LocalInvocationID.y * BOIL_FLT_GROUP_SIZE;
        const uint waveIndex = linearThreadIndex / gl_SubgroupSize;
        const uint4 threadMask = subgroupBallot(reservoirWeight > 0.0f);

        uint threadCount = subgroupBallotBitCount(threadMask);
        float waveWeight = subgroupAdd(reservoirWeight);

        if (subgroupElect())
        {
            s_weights[waveIndex] = waveWeight;
            s_count[waveIndex] = threadCount;
        }

        barrier();

        // Reduce the per-wavefront averages into a global average using one wavefront
        if (linearThreadIndex < (BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + gl_SubgroupSize - 1) / gl_SubgroupSize)
        {
            waveWeight = s_weights[linearThreadIndex];
            threadCount = s_count[linearThreadIndex];
            waveWeight = subgroupAdd(waveWeight);
            threadCount = subgroupAdd(threadCount);

            if (linearThreadIndex == 0)
            {
                s_weights[0] = lerp(0.0f, waveWeight / float(threadCount), threadCount > 0);
            }
        }

        barrier();

        isBoiling = reservoirWeight > s_weights[0] * boilingFilterMultiplier;
    }


    // Shade Hit & Store Temporal Reservoir

    // We should retrace visibility here as to not accumulate invalid samples.
    // However, this is expensive. Currently reservoirs are validated in async during present (which is virtually free).
    const bool reject = ReSTIR_NearFieldReject(depth, origin, initial, ReSTIR_Hash(seed + 1));
    const float3 combinedDirection = normalize(combined.position - origin);
    const float weight = ReSTIR_GetSampleWeight(combined, normal, combinedDirection);
    const bool isValid = !isnan(weight) && !isinf(weight) && weight > 0.0f && !reject;

    ReSTIR_Store_Current(baseCoord, isValid && !isBoiling ? combined : initial);

    const float3 radiance = lerp(initial.radiance, combined.radiance * weight, isValid.xxx);
    const float3 direction = lerp(safeNormalize(initial.position - origin), combinedDirection, isValid.xxx);
    return SH_FromRadiance(radiance, direction);
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const float4 normalroughness = SampleViewNormalRoughness(coord);
    const float depth = PK_GI_SAMPLE_DEPTH(coord);
    const bool isScene = Test_DepthFar(depth);

    // Diffuse 
    {
        const float3 origin = CoordToWorldPos(coord, depth);
        const Reservoir reservoir = ReSTIR_Load_HitAsReservoir(baseCoord, origin);
        const float4 samplevec = normalizeLength(reservoir.position - origin);

        GIDiff current;
        current.ao = min(1.0f, samplevec.w / PK_GI_RAY_TMAX);

#if defined(PK_GI_RESTIR)
        current.sh = ReSTIR_ResampleSpatioTemporal(baseCoord, coord, depth, normalroughness.xyz, origin, reservoir);
#else
        current.sh = SH_FromRadiance(reservoir.radiance, samplevec.xyz);
#endif

        GIDiff history = GI_Load_Diff(baseCoord, PK_GI_STORE_LVL);
        const float alpha = GI_Alpha(history);

        SUBGROUP_ANTIFIREFLY_FILTER(isScene, current, history, alpha, 1.0f)

        history = GI_Interpolate(history, current, GI_Alpha(history));
        GI_Store_Packed_Diff(baseCoord, isScene ? GI_Pack_Diff(history) : uint4(0));
    }

    // Specular
#if PK_GI_APPROX_ROUGH_SPEC == 1
    [[branch]]
    if (normalroughness.w < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        [[branch]]
        if (subgroupAny(isScene))
        {
            GISpec history = GI_Load_Spec(baseCoord, PK_GI_STORE_LVL);
            GISpec current = GI_Load_Spec(baseCoord);
            const float alpha = GI_Alpha(history);

            SUBGROUP_ANTIFIREFLY_FILTER(isScene, current, history, alpha, (1.0f / (1e-4f + normalroughness.w)))

            history = GI_Interpolate(history, current, alpha);
            GI_Store_Packed_Spec(baseCoord, isScene ? GI_Pack_Spec(history) : uint2(0));
        }
    }
}