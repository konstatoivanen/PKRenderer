#version 460
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
        (float((seed & 0x000000FF)) / float(255)) * 2.0f - 1.0f,
        (float((seed & 0x0000FF00) >> 8) / float(255)) * 2.0f - 1.0f,
        float((seed & 0x0000FFFF)) / float(65535)
    );
}

float GetInverseJacobianGeometryFactor(const float3 pos_center, const float3 pos_sample, const float3 pos_reservoir, const float3 nor_reservoir)
{
    float3 dir_center = pos_center - pos_reservoir;
    float3 dir_sample = pos_sample - pos_reservoir;
    float len_center = max(length(dir_center), 0.001f);
    float len_sample = max(length(dir_sample), 0.001f);
    dir_center /= len_center;
    dir_sample /= len_sample;

    float jacobian = 1.0f;
    jacobian *= safePositiveRcp(max(0.0f, dot(nor_reservoir, dir_center)) * safePositiveRcp(len_center * len_center));
    jacobian *= max(0.0f, dot(nor_reservoir, dir_sample)) * safePositiveRcp(len_sample * len_sample);
    jacobian = saturate(jacobian);
    return jacobian;
}

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 baseCoord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_16, PK_W_ALIGNMENT_8, 8u));
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

    Reservoir combined = Restir_Load(baseCoord, RESTIR_LAYER_HIT);

    int spatialSamplesCount = RESTIR_SAMPLES_SPATIAL;    
    {
        const float2 screenuvPrev = WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) - 0.49f.xx;
    
        for (int pixIndex = 0; pixIndex < RESTIR_SAMPLES_TEMPORAL; pixIndex++)
        {
            const float3 rnd = wellonsLowBias121(seed++);
            const int2 xyFull = int2(floor(screenuvPrev + rnd.xy * RESTIR_RADIUS_TEMPORAL));
            const int2 xy = GI_CollapseCheckerboardCoord(xyFull);

            Reservoir s_reservoir = Restir_Load(xy, RESTIR_LAYER_PRE);
            const float s_depth = SamplePreviousViewDepth(xyFull);
            const float3 s_normal = SamplePreviousWorldNormal(xyFull);
            const float3 s_sampledir = normalize(s_reservoir.selected.position - origin);
    
            if (Test_DepthFar(s_depth) && 
                Test_InScreen(xyFull) && 
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > 0.0f && 
                dot(normal, s_sampledir) > 0.0f)
            {
                NormalizeReservoir(s_reservoir, 24);
                UpdateCombinedReservoir(combined, s_reservoir, rnd.z);
            }
        }
    }

    {
        uint nobiasM = combined.M;
    
        for (int pixIndex = 0; pixIndex < spatialSamplesCount; pixIndex++)
        {
            const float3 rnd = wellonsLowBias121(seed++);
            const int2 xy = int2(floor(baseCoord + 0.5f.xx + rnd.xy * RESTIR_RADIUS_SPATIAL));
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);

            const Reservoir s_reservoir = Restir_Load(xy, RESTIR_LAYER_HIT);
            const float s_depth = SampleViewDepth(xyFull);
            const float3 s_normal = SampleWorldNormal(xyFull);
            const float3 s_sampledir = normalize(s_reservoir.selected.position - origin);
    
            if (Test_DepthFar(s_depth) &&
                Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias) &&
                dot(normal, s_normal) > 0.0f &&
                dot(normal, s_sampledir) > 0.0f)
            {
    
                float targetPdf_curSurf = GetSelectedTargetPdf(s_reservoir);

                targetPdf_curSurf *= GetInverseJacobianGeometryFactor
                (
                    origin,
                    SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth), 
                    s_reservoir.selected.position,
                    s_reservoir.selected.normal
                );

                UpdateCombinedReservoir_NewSurf(combined, s_reservoir, targetPdf_curSurf, rnd.z);
    
                if (targetPdf_curSurf > 0.0)
                {
                    nobiasM += s_reservoir.M;
                }
            }
        }
    
        combined.M = nobiasM;
    }

    Restir_Store(baseCoord, RESTIR_LAYER_CUR, combined);

    const float3 surfToHitPoint = normalize(combined.selected.position - origin);
    const float  nl = dot(normal, surfToHitPoint);
    const float3 radiance = combined.selected.radiance * GetSelectedSampleWeight(combined) * nl * PK_INV_PI; 

    GIDiff diff = GI_Load_Cur_Diff(coord);
    diff.sh = SH_FromRadiance(radiance, surfToHitPoint);
    GI_Store_Diff(coord, diff);
}