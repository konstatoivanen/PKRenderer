#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable

#pragma PROGRAM_COMPUTE
#multi_compile _ PK_GI_CHECKERBOARD_TRACE
#multi_compile _ PK_GI_RESTIR

#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedReSTIR.glsl
#include includes/CTASwizzling.glsl

uint MurmurHash13(uint3 src)
{
    const uint M = 0x5bd1e995u;
    uint h = 1190494759u;
    src *= M; src ^= src >> 24u; src *= M;
    h *= M; h ^= src.x; h *= M; h ^= src.y; h *= M; h ^= src.z;
    h ^= h >> 13u; h *= M; h ^= h >> 15u;
    return h;
}

// https://nullprogram.com/blog/2018/07/31/
uint WellonsLowBias(uint seed)
{
    seed ^= seed >> 16;
    seed *= 0x7feb352dU;
    seed ^= seed >> 15;
    seed *= 0x846ca68bU;
    seed ^= seed >> 16;
    return seed;
}

float UintToUnorm(uint x) { return uintBitsToFloat(x & 0x007fffffu | 0x3f800000u) - 1.0; }
int2 UshortToSnormInt16(uint x) { return int2((x & 0x0000003F) - 32, ((x & 0x00003F00) >> 8) - 32); }
int2 UshortToSnormInt8(uint x) { return int2((x & 0x0000001F) - 32, ((x & 0x00001F00) >> 8) - 16); }

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

void ReSTIR_ResampleSpatioTemporal(const int2 baseCoord, const int2 coord, const float depth, inout GIDiff diff)
{
    const float3 viewnormal = SampleViewNormal(coord);
    const float depthBias = lerp(0.1f, 0.01f, -viewnormal.z);
    const float3 normal = ViewToWorldDir(viewnormal);
    const float3 origin = SampleWorldPosition(coord, int2(pk_ScreenSize.xy), depth);
    uint seed = MurmurHash13(uint3(baseCoord, pk_FrameRandom.x + 132u));

    const Reservoir initial = ReSTIR_Load_HitAsReservoir(baseCoord, origin);

    Reservoir combined = initial;

    {
        const float2 screenUvPrev = WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) + RESTIR_TEXEL_BIAS;
        const int2 coordPrev = GI_CollapseCheckerboardCoord(screenUvPrev, 1u);

        for (int i = 0; i < RESTIR_SAMPLES_TEMPORAL; ++i)
        {
            const float random = UintToUnorm(WellonsLowBias(seed + i));
            const int2 xy = coordPrev + ReSTIR_CalculateTemporalResamplingOffset(int(seed) + i);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy, 1u);

            Reservoir s_reservoir = ReSTIR_Load(xy, RESTIR_LAYER_PRE);
            const float  s_depth = SamplePreviousViewDepth(xyFull);
            const float3 s_normal = SamplePreviousWorldNormal(xyFull);
            const float3 s_position = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
            const float3 s_direction = normalize(s_reservoir.position - origin);

            if (Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD &&
                s_reservoir.age < RESTIR_MAX_AGE)
            {
                ReSTIR_Normalize(s_reservoir, 20);
                float targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_CombineReservoir(combined, s_reservoir, targetPdf, random);
            }
        }
    }

    {
        uint nobiasM = combined.M;

        for (uint i = 0u; i < RESTIR_SAMPLES_SPATIAL; ++i)
        {
            const uint hash = WellonsLowBias(seed++);
            const float random = UintToUnorm(hash);
            const int2 xy = baseCoord + UshortToSnormInt8(hash);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);

            const float  s_depth = SampleMinZ(xyFull, 0);
            const float3 s_normal = SampleWorldNormal(xyFull);
            const float3 s_position = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
            const Reservoir s_reservoir = ReSTIR_Load_HitAsReservoir(xy, s_position);

            if (Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD)
            {
                float targetPdf = ReSTIR_GetTargetPdfNewSurf(origin, normal, s_position, s_reservoir);
                ReSTIR_CombineReservoir(combined, s_reservoir, targetPdf, random);

                if (targetPdf > 0.0)
                {
                    nobiasM += s_reservoir.M;
                }
            }
        }

        combined.M = nobiasM;
    }

    combined.age++;

    // Reshade hit
    bool isValid;
    {
        const float3 surfToHitPoint = normalize(combined.position - origin);
        const float  nl = dot(normal, surfToHitPoint);
        const float3 radiance = combined.radiance * ReSTIR_GetSampleWeight(combined) * nl * PK_INV_PI;
        const float random01 = UintToUnorm(WellonsLowBias(seed));
        const bool isNear = ReSTIR_IsNearField(depth, origin, initial, random01);
        isValid = !Any_IsNaN(radiance);

        if (isValid && (diff.history > 32.0f || !isNear))
        {
            diff.sh = SH_FromRadiance(radiance, surfToHitPoint);
        }
    }

    if (ReSTIR_BoilingFilter(gl_LocalInvocationID.xy, 0.2f, combined.targetPdf * combined.weightSum) || !isValid)
    {
        combined = pk_Reservoir_Zero;
    }

    ReSTIR_Store(baseCoord, RESTIR_LAYER_CUR, combined);
    ReSTIR_Store(baseCoord, RESTIR_LAYER_PRE, combined);
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const float depth = SampleMinZ(coord, 0);

    [[branch]]
    if (!Test_DepthFar(depth))
    {
        return;
    }

    // Specular
#if PK_GI_APPROX_ROUGH_SPEC == 1
    [[branch]]
    if (SampleRoughness(coord) < PK_GI_MAX_ROUGH_SPEC)
#endif
    {
        GISpec spec = GI_Load_Cur_Spec(coord);
        GISpec specSample = GI_Load_Spec(coord);

        const float wSpec = max(1.0f / (spec.history + 1.0f), PK_GI_MIN_ACCUM);
        const float maxLumaSpec = GI_Luminance(spec) + (PK_GI_MAX_LUMA_GAIN / (1.0f - wSpec));
        const float sampleLumaSpec = GI_Luminance(specSample);
        const float scaleSpec = (min(sampleLumaSpec, maxLumaSpec) + 1e-6f) / (sampleLumaSpec + 1e-6f);
        specSample.radiance *= scaleSpec;
    
        spec.radiance = lerp(spec.radiance, specSample.radiance, wSpec);
        spec.ao = lerp(spec.ao, specSample.ao, wSpec);
        GI_Store_Spec(coord, spec);
    }

    // Diffuse 
    GIDiff diff = GI_Load_Cur_Diff(coord);
    {
        GIDiff diffSample = GI_Load_Diff(coord);

        // ReSTIR
        #if defined(PK_GI_RESTIR)
        ReSTIR_ResampleSpatioTemporal(baseCoord, coord, depth, diffSample);
        #endif

        const float wDiff = max(1.0f / (diff.history + 1.0f), PK_GI_MIN_ACCUM);
        const float maxLumaDiff = GI_Luminance(diff) + (PK_GI_MAX_LUMA_GAIN / (1.0f - wDiff));
        const float sampleLumaDiff = GI_Luminance(diffSample);
        const float scaleDiff = (min(sampleLumaDiff, maxLumaDiff) + 1e-6f) / (sampleLumaDiff + 1e-6f);
        diffSample.sh.Y *= scaleDiff;
        diffSample.sh.CoCg *= scaleDiff;
        
        diff.sh = SH_Interpolate(diff.sh, diffSample.sh, wDiff);
        diff.ao = lerp(diff.ao, diffSample.ao, wDiff);
        GI_Store_Diff(coord, diff);
    }
}