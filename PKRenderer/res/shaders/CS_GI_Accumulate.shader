#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_RESTIR

#define PK_GI_LOAD_LVL 1
#define PK_GI_STORE_LVL 0

#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedReSTIR.glsl

#define BOIL_FLT_GROUP_SIZE PK_W_ALIGNMENT_8
#define BOIL_FLT_MIN_LANE_COUNT 32
shared float s_weights[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];
shared uint s_count[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];

float SSRT_ValidateVisibility(const float3 start_ws, const float3 end_ws, const uint max_count)
{
    const float Z_THICKNESS = 0.05f;

    float4 start_cs = WorldToClipPos(start_ws);
    start_cs.xyz /= start_cs.w;
    float4 end_cs = WorldToClipPos(end_ws);
    end_cs.xyz /= end_cs.w;

    const float2 delta_px = (end_cs.xy - start_cs.xy) * 0.5f * (pk_ScreenSize.xy / 2);
    const uint count = min(max_count, uint(length(delta_px) / 2u));
    const float z_step = (end_cs.z - start_cs.z) / length(end_cs.xy - start_cs.xy);
    const float t_step = 1.0f / count;
    
    float t = 0.5f * t_step;
    float visibility = 1.0f;

    for (uint k = 0u; k < count; ++k)
    {
        const float3 clippos = lerp(start_cs.xyz, end_cs.xyz, t);
        const float2 uv = clippos.xy * 0.5f + 0.5f;
        const uint2 coord = uint2(uv * pk_ScreenSize.xy);
        const float depth = ClipDepth(SampleAvgZ(coord >> 1u, 1u));
        const float2 quantized = ((coord + 0.5f.xx) / pk_ScreenSize.xy) * 2.0f - 1.0f;
        const float biased_z = start_cs.z + z_step * length(quantized - start_cs.xy);

        if (depth > biased_z && Test_InScreen(uv))
        {
            const float diff = abs(max(1e-20, clippos.z) / max(1e-20, depth) - 1.0);
            const float hit = smoothstep(Z_THICKNESS, Z_THICKNESS * 0.5f, diff);
            visibility *= 1.0f - hit;
        }

        t += t_step;
    }

    return visibility;
}

Reservoir ReSTIR_Load_HitAsReservoir(const int2 coord, const float3 origin)
{
    return ReSTIR_Unpack_Hit(GI_Load_Packed_Diff(coord), origin);
}

bool ReSTIR_BoilingFilter(uint2 LocalIndex, float filterStrength, float reservoirWeight)
{
    float boilingFilterMultiplier = 10.f / clamp(filterStrength, 1e-6, 1.0) - 9.f;
    float waveWeight = subgroupAdd(reservoirWeight);
    uint4 weightMask = subgroupBallot(reservoirWeight > 0.0f);
    uint waveCount = subgroupBallotBitCount(weightMask);
    uint linearThreadIndex = LocalIndex.x + LocalIndex.y * BOIL_FLT_GROUP_SIZE;
    uint waveIndex = linearThreadIndex / gl_SubgroupSize;

    if (subgroupElect())
    {
        s_weights[waveIndex] = waveWeight;
        s_count[waveIndex] = waveCount;
    }

    barrier();

    // Reduce the per-wavefront averages into a global average using one wavefront
    if (linearThreadIndex < (BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + gl_SubgroupSize - 1) / gl_SubgroupSize)
    {
        waveWeight = s_weights[linearThreadIndex];
        waveCount = s_count[linearThreadIndex];
        waveWeight = subgroupAdd(waveWeight);
        waveCount = subgroupAdd(waveCount);

        if (linearThreadIndex == 0)
        {
            s_weights[0] = (waveCount > 0) ? (waveWeight / float(waveCount)) : 0.0;
        }
    }

    barrier();

    float averageNonzeroWeight = s_weights[0];
    return reservoirWeight > averageNonzeroWeight * boilingFilterMultiplier;
}

void ReSTIR_SubgroupSuffle(const int2 coord, 
                           const float depth,
                           const float depthBias,
                           const float3 origin, 
                           const float3 normal, 
                           const uint hash,
                           inout Reservoir combined)
{
    const uint subgroupMask = gl_SubgroupSize - 1u;
    const uint mask = (gl_SubgroupInvocationID + (hash % subgroupMask) + 1u) & subgroupMask;

    Reservoir suffled;
    suffled.radiance = subgroupShuffle(combined.radiance, mask);
    suffled.position = subgroupShuffle(combined.position, mask);
    suffled.normal = subgroupShuffle(combined.normal, mask);
    suffled.targetPdf = subgroupShuffle(combined.targetPdf, mask);
    suffled.weightSum = subgroupShuffle(combined.weightSum, mask);
    suffled.M = subgroupShuffle(combined.M, mask);

    const float s_depth = subgroupShuffle(depth, mask);
    const float3 s_origin = subgroupShuffle(origin, mask);
    const float3 s_normal = subgroupShuffle(normal, mask);
    const int2 s_coord = subgroupShuffle(coord, mask);

    // Also check screen area in case receiving invalid values.

    [[branch]]
    if (Test_InScreen(s_coord) &&
        combined.M < RESTIR_MAX_M + 3 &&
        Test_DepthReproject(depth, s_depth, depthBias) &&
        dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
    {
        const float3 s_position = SampleWorldPosition(s_coord, s_depth);
        const float s_targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_origin, suffled);
        ReSTIR_CombineReservoir(combined, suffled, s_targetPdf, hash);
    }
}

void ReSTIR_ResampleSpatioTemporal(const int2 baseCoord, 
                                   const int2 coord, 
                                   const float depth,
                                   const float3 origin,
                                   const bool isNewSurf,
                                   const Reservoir initial,
                                   inout SH outSH)
{
    const float3 viewnormal = SampleViewNormal(coord);
    const float depthBias = lerp(0.1f, 0.01f, -viewnormal.z);
    const float3 normal = ViewToWorldDir(viewnormal);
    uint seed = ReSTIR_GetSeed(coord);
    int scale = 5;

    Reservoir combined = initial;

    // Temporal Resampling
    {
        const float2 screenUvPrev = WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) + RESTIR_TEXEL_BIAS;
        const int2 coordPrev = GI_CollapseCheckerboardCoord(screenUvPrev, 1u);
        const uint hash = ReSTIR_Hash(seed++);

        for (uint i = 0u; i < RESTIR_SAMPLES_TEMPORAL; ++i)
        {
            scale = int(RESTIR_SAMPLES_TEMPORAL - i - 1);
            const int2 xy = ReSTIR_GetTemporalResamplingCoord(coordPrev, int(hash + i), scale, i == 0);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy, 1u);
            const float s_depth = SamplePreviousViewDepth(xyFull);
            const float3 s_normal = SamplePreviousViewNormal(xyFull);

            if (Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(viewnormal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                // Don't sample multiple temporal reservoirs to avoid boiling. Break on first accepted sample.
                Reservoir s_reservoir = ReSTIR_Load(xy, RESTIR_LAYER_PRE);
                ReSTIR_Normalize(s_reservoir, RESTIR_MAX_M);
                ReSTIR_CombineReservoirSimple(combined, s_reservoir, hash);
                break;
            }
        }

        scale = clamp(scale, 2, 5);
    }

    // Spatial Resampling
    {
        [[loop]]
        for (uint i = 0u; i < RESTIR_SAMPLES_SPATIAL; ++i)
        {
            const uint hash = ReSTIR_Hash(seed + i);
            const int2 xy = ReSTIR_GetSpatialResamplingCoord(baseCoord, scale, hash);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);
            const float s_depth = SampleMinZ(xyFull, 0);
            const float3 s_normal = SampleWorldNormal(xyFull);
            const float3 s_position = SampleWorldPosition(xyFull, s_depth);
            const Reservoir s_reservoir = ReSTIR_Load_HitAsReservoir(xy, s_position);

            [[branch]]
            if (Any_NotEqual(xy, baseCoord) &&
                Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                const float s_targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_CombineReservoir(combined, s_reservoir, s_targetPdf, hash);
            }
        }

        seed += RESTIR_SAMPLES_SPATIAL;
    }

    ReSTIR_SubgroupSuffle(coord, depth, depthBias, origin, normal, ReSTIR_Hash(seed), combined);

    // Reshade hit
    {
        // Ignore visibility for disocclusions
        const float visibility = isNewSurf ? 1.0f : SSRT_ValidateVisibility(origin, combined.position, 6);
        const bool reject = !isNewSurf && ReSTIR_NearFieldReject(depth, origin, initial, ReSTIR_Hash(seed + 1));
        const float3 direction = normalize(combined.position - origin);
        const float weight = ReSTIR_GetSampleWeight(combined, normal, direction);
        const bool isValid = !isnan(weight) && !isinf(weight) && weight > 0.0f && !reject;

        if (isValid)
        {
            SH newSH = SH_FromRadiance(combined.radiance * weight, direction);
            outSH = SH_Interpolate(outSH, newSH, visibility);
        }

        if (ReSTIR_BoilingFilter(gl_LocalInvocationID.xy, 0.2f, combined.targetPdf * combined.weightSum) || !isValid)
        {
            combined = initial;
        }
    }

    ReSTIR_Store(baseCoord, RESTIR_LAYER_CUR, combined);
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(gl_GlobalInvocationID.xy);
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const float depth = SampleMinZ(coord, 0);
    const bool isScene = Test_DepthFar(depth);

    // Diffuse 
    {
        const float3 origin = SampleWorldPosition(coord, depth);
        const Reservoir reservoir = ReSTIR_Load_HitAsReservoir(baseCoord, origin);
        const float4 samplevec = normalizeLength(reservoir.position - origin);

        GIDiff current;
        current.sh = SH_FromRadiance(reservoir.radiance, samplevec.xyz);
        current.ao = saturate(samplevec.w / PK_GI_RAY_TMAX);

        GIDiff history = GI_Load_Diff(baseCoord, PK_GI_STORE_LVL);

        #if defined(PK_GI_RESTIR)
        ReSTIR_ResampleSpatioTemporal(baseCoord, coord, depth, origin, int(history.history) < 4, reservoir, current.sh);
        #endif

        const float alpha = GI_Alpha(history);
        current = GI_ClampLuma(current, GI_MaxLuma(history, alpha));
        history = GI_Interpolate(history, current, alpha);
        GI_Store_Packed_Diff(baseCoord, isScene ? GI_Pack_Diff(history) : uint4(0));
    }

    // Specular
    #if PK_GI_APPROX_ROUGH_SPEC == 1
    [[branch]]
    if (SampleRoughness(coord) < PK_GI_MAX_ROUGH_SPEC)
    #endif
    {
        GISpec history = GI_Load_Spec(baseCoord, PK_GI_STORE_LVL);
        GISpec current = GI_Load_Spec(baseCoord);
        const float alpha = GI_Alpha(history);
        current = GI_ClampLuma(current, GI_MaxLuma(history, alpha));
        history = GI_Interpolate(history, current, alpha);
        GI_Store_Packed_Spec(baseCoord, isScene ? GI_Pack_Spec(history) : uint2(0));
    }
}