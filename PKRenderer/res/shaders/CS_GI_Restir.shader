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

int2 CalculateTemporalResamplingOffset(int sampleIdx, int radius)
{
    sampleIdx &= 7;
    int mask2 = sampleIdx >> 1 & 0x01;       
    int mask4 = 1 - (sampleIdx >> 2 & 0x01); 
    int tmp0 = -1 + 2 * (sampleIdx & 0x01);  
    int tmp1 = 1 - 2 * mask2;                
    int tmp2 = mask4 | mask2;                
    int tmp3 = mask4 | (1 - mask2);          
    return int2(tmp0, tmp0 * tmp1) * int2(tmp2, tmp3) * radius;
}

#define BOIL_FLT_GROUP_SIZE PK_W_ALIGNMENT_8
#define BOIL_FLT_MIN_LANE_COUNT 32
shared float s_weights[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];
shared uint s_count[(BOIL_FLT_GROUP_SIZE * BOIL_FLT_GROUP_SIZE + BOIL_FLT_MIN_LANE_COUNT - 1) / BOIL_FLT_MIN_LANE_COUNT];

bool BoilingFilter(uint2 LocalIndex, float filterStrength, float reservoirWeight)
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

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    const int2 coord = GI_ExpandCheckerboardCoord(uint2(baseCoord));
    const float depth = SampleViewDepth(coord);

    if (!Test_DepthFar(depth))
    {
        Restir_Store_Empty(baseCoord, RESTIR_LAYER_CUR);
        return;
    }

    const float4 normalRoughness = SampleWorldNormalRoughness(coord);                      
    const float3 normal = normalRoughness.xyz;
    const float depthBias = lerp(0.1f, 0.01f, -normal.z);
    const float roughness = normalRoughness.w;
    const float3 origin = SampleWorldPosition(coord, int2(pk_ScreenSize.xy), depth);
    uint seed = MurmurHash13(uint3(baseCoord, pk_FrameRandom.x + 132u));

    Reservoir combined = Restir_Load_HitAsReservoir(baseCoord, origin);

    {
        const float random = UintToUnorm(seed);
        const int2 coordPrev = int2(WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) + RESTIR_TEXEL_BIAS);
        const int2 xy = GI_CollapseCheckerboardCoord(int2(coordPrev), 1u) + CalculateTemporalResamplingOffset(int(seed), 2);
        const int2 xyFull = GI_ExpandCheckerboardCoord(xy, 1u);

        Reservoir s_reservoir = Restir_Load(xy, RESTIR_LAYER_PRE);
        const float  s_depth = SamplePreviousViewDepth(xyFull);
        const float3 s_normal = SamplePreviousWorldNormal(xyFull);
        const float3 s_direction = normalize(s_reservoir.position - origin);

        if (Test_InScreen(xyFull) && 
            Test_DepthReproject(depth, s_depth, depthBias) &&
            dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD &&
            dot(normal, s_direction) > 0.05f &&
            s_reservoir.age < RESTIR_MAX_AGE)
        {
            Restir_Normalize(s_reservoir, 20);
            Restir_CombineReservoirTemporal(combined, s_reservoir, random);
        }
    }

    {
        uint nobiasM = combined.M;
    
        for (uint i = 0u; i < RESTIR_SAMPLES_SPATIAL; ++i)
        {
            const uint hash = WellonsLowBias(seed++);
            const float random = UintToUnorm(hash);
            const int2 xy = baseCoord + UshortToSnormInt16(hash);
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);

            const float  s_depth = SampleViewDepth(xyFull);
            const float3 s_normal = SampleWorldNormal(xyFull);
            const float3 s_position = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
            const Reservoir s_reservoir = Restir_Load_HitAsReservoir(xy, s_position);
            const float3 s_direction = normalize(s_reservoir.position - origin);
    
            if (Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > RESTIR_NORMAL_THRESHOLD &&
                dot(normal, s_direction) > 0.05f)
            {
                float targetPdf = Restir_GetTargetPdf(s_reservoir) * Restir_GetJacobian(origin, s_position, s_reservoir);
                Restir_CombineReservoirSpatial(combined, s_reservoir, targetPdf, random);
    
                if (targetPdf > 0.0)
                {
                    nobiasM += s_reservoir.M;
                }
            }
        }
    
        combined.M = nobiasM;
    }

    combined.age++;

    const float3 surfToHitPoint = normalize(combined.position - origin);
    const float  nl = dot(normal, surfToHitPoint);
    const float3 radiance = combined.radiance * Restir_GetSampleWeight(combined) * nl * PK_INV_PI; 
    const bool isValid = !Any_IsNaN(radiance);

    GIDiff diff = GI_Load_Cur_Diff(coord);
    
    if (isValid)
    {
        diff.sh = SH_FromRadiance(radiance, surfToHitPoint);
    }

    if (BoilingFilter(gl_LocalInvocationID.xy, 0.2f, Restir_GetTargetPdf(combined) * combined.weightSum) || !isValid)
    {
        combined = pk_Reservoir_Zero;
    }

    GI_Store_Diff(coord, diff);
    Restir_Store(baseCoord, RESTIR_LAYER_CUR, combined);
}