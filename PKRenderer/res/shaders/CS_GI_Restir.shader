#version 460
#extension GL_KHR_shader_subgroup_ballot : enable
#extension GL_KHR_shader_subgroup_arithmetic : enable

#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedRestir.glsl
#include includes/CTASwizzling.glsl

uint murmurHash13(uint3 src) 
{
    const uint M = 0x5bd1e995u;
    uint h = 1190494759u;
    src *= M; 
    src ^= src >> 24u; 
    src *= M;
    h *= M; 
    h ^= src.x; 
    h *= M; 
    h ^= src.y;
    h *= M; 
    h ^= src.z;
    h ^= h >> 13u; 
    h *= M; 
    h ^= h >> 15u;
    return h;
}

// https://nullprogram.com/blog/2018/07/31/
float3 wellonsLowBias121(uint seed)
{
    seed ^= seed >> 16;
    seed *= 0x7feb352dU;
    seed ^= seed >> 15;
    seed *= 0x846ca68bU;
    seed ^= seed >> 16;
    
    return float3
    (
        float((seed & 0x000000FF)) / float(255),
        float((seed & 0x0000FF00) >> 8) / float(255),
        float((seed & 0x0000FFFF)) / float(65535)
    );
}

// RTXDI_BOILING_FILTER_GROUP_SIZE must be defined - 16 is a reasonable value
#define RTXDI_BOILING_FILTER_GROUP_SIZE PK_W_ALIGNMENT_8
#define RTXDI_BOILING_FILTER_MIN_LANE_COUNT 32

shared float s_weights[(RTXDI_BOILING_FILTER_GROUP_SIZE * RTXDI_BOILING_FILTER_GROUP_SIZE + RTXDI_BOILING_FILTER_MIN_LANE_COUNT - 1) / RTXDI_BOILING_FILTER_MIN_LANE_COUNT];
shared uint s_count[(RTXDI_BOILING_FILTER_GROUP_SIZE * RTXDI_BOILING_FILTER_GROUP_SIZE + RTXDI_BOILING_FILTER_MIN_LANE_COUNT - 1) / RTXDI_BOILING_FILTER_MIN_LANE_COUNT];

bool BoilingFilter(uint2 LocalIndex, float filterStrength, float reservoirWeight)
{
    float boilingFilterMultiplier = 10.f / clamp(filterStrength, 1e-6, 1.0) - 9.f;

    // Start with average nonzero weight within the wavefront
    float waveWeight = subgroupAdd(reservoirWeight);
    uint4 weightMask = subgroupBallot(reservoirWeight > 0.0f);
    uint waveCount = subgroupBallotBitCount(weightMask);

    // Store the results of each wavefront into shared memory
    uint linearThreadIndex = LocalIndex.x + LocalIndex.y * RTXDI_BOILING_FILTER_GROUP_SIZE;
    uint waveIndex = linearThreadIndex / gl_SubgroupSize;

    if (subgroupElect())
    {
        s_weights[waveIndex] = waveWeight;
        s_count[waveIndex] = waveCount;
    }

    barrier();

    // Reduce the per-wavefront averages into a global average using one wavefront
    if (linearThreadIndex < (RTXDI_BOILING_FILTER_GROUP_SIZE * RTXDI_BOILING_FILTER_GROUP_SIZE + gl_SubgroupSize - 1) / gl_SubgroupSize)
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

    // Read the per-group average and apply the threshold
    float averageNonzeroWeight = s_weights[0];
    if (reservoirWeight > averageNonzeroWeight * boilingFilterMultiplier)
    {
        return true;
    }

    return false;
}

void BoilingFilter(inout Reservoir r)
{
    float weight = Restir_GetTargetPdf(r) * r.weightSum;

    if (BoilingFilter(gl_LocalInvocationID.xy, 0.2f, weight))
    {
        r = pk_Reservoir_Zero;
    }

    // reset reservoirs that have been sampled too many times.
    if (r.M > 20)
    {
        r = pk_Reservoir_Zero;
    }
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const float depth = SampleViewDepth(coord);

    if (!Test_DepthFar(depth))
    {
        Restir_Store_Packed(baseCoord, RESTIR_LAYER_CUR, pk_PackedReservoir_Zero);
        return;
    }

    const float4 normalRoughness = SampleWorldNormalRoughness(coord);                      
    const float3 normal = normalRoughness.xyz;
    const float depthBias = lerp(0.1f, 0.01f, -normal.z);
    const float roughness = normalRoughness.w;
    const float3 origin = SampleWorldPosition(coord, int2(pk_ScreenSize.xy), depth);
    uint seed = murmurHash13(uint3(baseCoord, pk_FrameRandom.x + 132u));

    Reservoir combined = Restir_Load_HitAsReservoir(baseCoord, origin, normal);

    int spatialSamplesCount = RESTIR_SAMPLES_SPATIAL;    
    {
        const float3 rnd = wellonsLowBias121(seed++);
        const float2 screenuvPrev = WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) + float2(0.49f, 0.0f);
        const int2 xyFull = int2(screenuvPrev + (rnd.xy * 4.0f - 2.0f));
        const int2 xy = GI_CollapseCheckerboardCoord(xyFull);

        Reservoir s_reservoir = Restir_Load(xy, RESTIR_LAYER_PRE);
        const float  s_depth = SamplePreviousViewDepth(xyFull);
        const float3 s_normal = SamplePreviousWorldNormal(xyFull);
        const float3 s_direction = normalize(s_reservoir.position - origin);
        const float3 s_position = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);

        if (Test_DepthFar(s_depth) && 
            Test_InScreen(xyFull) && 
            Test_DepthReproject(depth, s_depth, depthBias) &&
            dot(normal, s_normal) > 0.5f && 
            dot(normal, s_direction) > 0.0f)
        {
            float targetPdf = Restir_GetTargetPdf(s_reservoir) * Restir_GetJacobian(origin, s_position, s_reservoir);
            Restir_CombineReservoir(combined, s_reservoir, targetPdf, rnd.z);
        }
    }

    {
        uint nobiasM = combined.M;
    
        for (int pixIndex = 0; pixIndex < spatialSamplesCount; pixIndex++)
        {
            const float3 rnd = wellonsLowBias121(seed++);
            const int2 xy = int2(floor(baseCoord + 0.5f.xx + (rnd.xy * 2.0f - 1.0f) * RESTIR_RADIUS_SPATIAL));
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);

            const Reservoir s_reservoir = Restir_Load_HitAsReservoir(xy, origin, normal);
            const float s_depth = SampleViewDepth(xyFull);
            const float3 s_normal = SampleWorldNormal(xyFull);
            const float3 s_direction = normalize(s_reservoir.position - origin);
            const float3 s_position = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
    
            if (Test_DepthFar(s_depth) &&
                Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > 0.5f &&
                dot(normal, s_direction) > 0.0f)
            {
                float targetPdf = Restir_GetTargetPdf(s_reservoir) * Restir_GetJacobian(origin, s_position, s_reservoir);
                Restir_CombineReservoir(combined, s_reservoir, targetPdf, rnd.z);
    
                if (targetPdf > 0.0)
                {
                    nobiasM += s_reservoir.M;
                }
            }
        }
    
        combined.M = nobiasM;
    }

    const float3 surfToHitPoint = normalize(combined.position - origin);
    const float  nl = dot(normal, surfToHitPoint);
    const float3 radiance = combined.radiance * Restir_GetSampleWeight(combined) * nl * PK_INV_PI; 

    GIDiff diff = GI_Load_Cur_Diff(coord);
    diff.sh = SH_FromRadiance(radiance, surfToHitPoint);
    GI_Store_Diff(coord, diff);

    BoilingFilter(combined);
    Restir_Store(baseCoord, RESTIR_LAYER_CUR, combined);
}