#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable

#pragma PROGRAM_COMPUTE
#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_RESTIR

#define PK_GI_LOAD_LVL 2
#define PK_GI_STORE_LVL 0

#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedReSTIR.glsl

#define BOIL_FLT_GROUP_SIZE PK_W_ALIGNMENT_8
#define BOIL_FLT_MIN_LANE_COUNT 32
shared float s_weights[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];
shared uint s_count[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];

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

void ReSTIR_ResampleSpatioTemporal(const int2 baseCoord, 
                                   const int2 coord, 
                                   const float depth,
                                   const float3 origin,
                                   const Reservoir initial,
                                   inout SH outSH)
{
    const float3 viewnormal = SampleViewNormal(coord);
    const float depthBias = lerp(0.1f, 0.01f, -viewnormal.z);
    const float3 normal = ViewToWorldDir(viewnormal);
    uint seed = ReSTIR_GetSeed(baseCoord);

    Reservoir combined = initial;

    // Temporal Resampling
    {
        const float2 screenUvPrev = WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) + RESTIR_TEXEL_BIAS;
        const int2 coordPrev = GI_CollapseCheckerboardCoord(screenUvPrev, 1u);
        const uint hash = ReSTIR_Hash(seed++);

        for (uint i = 0u; i < RESTIR_SAMPLES_TEMPORAL; ++i)
        {
            const int2 xy = ReSTIR_GetTemporalResamplingCoord(coordPrev, int(hash + i), i == 0, i == 2);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy, 1u);
            const float s_depth = SamplePreviousViewDepth(xyFull);
            const float3 s_normal = SamplePreviousWorldNormal(xyFull);

            if (Any_NotEqual(xy, coordPrev) &&
                Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                // Don't sample multiple temporal reservoirs to avoid boiling. Break on first accepted sample.
                Reservoir s_reservoir = ReSTIR_Load(xy, RESTIR_LAYER_PRE);
                const float3 s_position = SamplePreviousWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
                const float s_targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_Normalize(s_reservoir, RESTIR_MAX_M);
                ReSTIR_CombineReservoir(combined, s_reservoir, s_targetPdf, hash);
                break;
            }
        }
    }

    // Spatial Resampling
    {
        [[loop]]
        for (uint i = 0u; i < RESTIR_SAMPLES_SPATIAL; ++i)
        {
            const uint hash = ReSTIR_Hash(seed + i);
            const int2 xy = ReSTIR_GetSpatialResamplingCoord(baseCoord, hash);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);
            const float s_depth = SampleMinZ(xyFull, 0);
            const float3 s_normal = SampleWorldNormal(xyFull);

            [[branch]]
            if (Any_NotEqual(xy, baseCoord) &&
                Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                const float3 s_position = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
                const Reservoir s_reservoir = ReSTIR_Load_HitAsReservoir(xy, s_position);
                const float s_targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_CombineReservoir(combined, s_reservoir, s_targetPdf, hash);
            }
        }

        seed += RESTIR_SAMPLES_SPATIAL;
    }

    // Reshade hit
    {
        const float nearField = ReSTIR_GetNearField(depth, origin, initial);
        const float3 sampledir = normalize(combined.position - origin);

        const float3 radiance = combined.radiance * 
                                ReSTIR_GetSampleWeight(combined) * 
                                dot(normal, sampledir) * 
                                PK_INV_PI;

        const bool isValid = !Any_IsNaN(radiance);

        if (isValid)
        {
            SH newSH = SH_FromRadiance(radiance, sampledir);
            outSH = SH_Interpolate(outSH, newSH, nearField);
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

    [[branch]]
    if (!Test_DepthFar(depth))
    {
        return;
    }

    // Diffuse 
    {
        const float3 origin = SampleWorldPosition(coord, int2(pk_ScreenSize.xy), depth);
        Reservoir reservoir = ReSTIR_Load_HitAsReservoir(baseCoord, origin);
        const float4 samplevec = normalizeLength(reservoir.position - origin);

        GIDiff current;
        current.sh = SH_FromRadiance(reservoir.radiance, samplevec.xyz);
        current.ao = saturate(samplevec.w / PK_GI_RAY_MAX_DISTANCE);

        // ReSTIR
        #if defined(PK_GI_RESTIR)
        ReSTIR_ResampleSpatioTemporal(baseCoord, coord, depth, origin, reservoir, current.sh);
        #endif

        GIDiff history = GI_Load_Cur_Diff(coord);

        const float alpha = GI_Alpha(history);
        const float maxLuma = GI_Luminance(history) + (PK_GI_MAX_LUMA_GAIN / (1.0f - alpha));
        current = GI_ClampLuma(current, maxLuma);
        
        history.sh = SH_Interpolate(history.sh, current.sh, alpha);
        history.ao = lerp(history.ao, current.ao, alpha);
        GI_Store_Diff(coord, history);
    }

    // Specular
#if PK_GI_APPROX_ROUGH_SPEC == 1
    [[branch]]
    if (SampleRoughness(coord) < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        GISpec history = GI_Load_Cur_Spec(coord);
        GISpec current = GI_Load_Spec(baseCoord);

        const float alpha = GI_Alpha(history);
        const float maxLuma = GI_Luminance(history) + (PK_GI_MAX_LUMA_GAIN / (1.0f - alpha));
        current = GI_ClampLuma(current, maxLuma);
    
        history.radiance = lerp(history.radiance, current.radiance, alpha);
        history.ao = lerp(history.ao, current.ao, alpha);
        GI_Store_Spec(coord, history);
    }
}