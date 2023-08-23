#version 460
#pragma PROGRAM_COMPUTE

#multi_compile _ PK_GI_CHECKERBOARD_TRACE

#include includes/GBuffers.glsl
#include includes/SceneEnv.glsl
#include includes/SharedSceneGI.glsl
#include includes/SharedRestir.glsl
#include includes/CTASwizzling.glsl

// https://nullprogram.com/blog/2018/07/31/
uint wellonsLowBias32(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

float4 rnd8_4(uint seed)
{
    uint rnd = wellonsLowBias32(seed);
    return float4
    (
        float((rnd & 0x000000FF)) / float(255),
        float((rnd & 0x0000FF00) >> 8) / float(255),
        float((rnd & 0x00FF0000) >> 16) / float(255),
        float((rnd & 0xFF000000) >> 24) / float(255)
    );
}

float rnd16(uint seed) { return float((wellonsLowBias32(seed) & 0x0000FFFF)) / float(65535); }

float getGeometryFactorClamped(const float3 lightNormal, const float3 lightToSurface, float surfaceToLightDistance)
{
    return max(0.0, dot(lightNormal, lightToSurface)) * safePositiveRcp(pow2(surfaceToLightDistance));
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
    const float3 viewdir = normalize(origin - pk_WorldSpaceCameraPos.xyz);
    int seed = int(pk_FrameRandom.x) + 132;

    Reservoir combined = Restir_Load(baseCoord, RESTIR_LAYER_HIT);

    int spatialSamplesCount = RESTIR_SAMPLES_SPATIAL;
    
    {
        const float2 screenuvPrev = WorldToPrevClipUV(origin) * int2(pk_ScreenSize.xy) - 0.49f.xx;
    
        for (int pixIndex = 0; pixIndex < RESTIR_SAMPLES_TEMPORAL; pixIndex++)
        {
            const float2 rndOffset = rnd8_4(seed++).xy * 2.0f - 1.0f;
            const float rnd = rnd16(seed++);
            const int2 xyFull = int2(floor(screenuvPrev + rndOffset * RESTIR_RADIUS_TEMPORAL));
            const int2 xy = GI_CollapseCheckerboardCoord(xyFull);
            const float s_depth = SamplePreviousViewDepth(xyFull);
            const float3 s_normal = SamplePreviousWorldNormal(xyFull);
    
            if (Test_DepthFar(s_depth) && 
                dot(normal, s_normal) > 0.0f && 
                Test_InScreen(xyFull) && 
                Test_DepthReproject(depth, s_depth, depthBias))
            {
                Reservoir temporal = Restir_Load(xy, RESTIR_LAYER_PRE);
                NormalizeReservoir(temporal, 24);
                UpdateCombinedReservoir(combined, temporal, rnd);
            }
        }
    }

    {
        uint nobiasM = combined.M;
    
        for (int pixIndex = 0; pixIndex < spatialSamplesCount; pixIndex++)
        {
            const float rnd = rnd16(seed++);
            const float2 rndOffset = rnd8_4(seed++).xy * 2.0f - 1.0f;
            const int2 xy = int2(floor(baseCoord + 0.5f.xx + rndOffset * RESTIR_RADIUS_SPATIAL));
            const int2 xyFull = GI_ExpandCheckerboardCoord(xy);
            const float s_depth = SampleViewDepth(xyFull);
            const float3 s_normal = SampleWorldNormal(xyFull);
    
            if (Test_DepthFar(s_depth) &&
                dot(normal, s_normal) > 0.0f &&
                Test_InScreen(xyFull) &&
                Test_DepthReproject(depth, s_depth, depthBias))
            {
                Reservoir reservoir_q = Restir_Load(xy, RESTIR_LAYER_HIT);
    
                float oneOverJacobian = 1.0f;
                {
                    const float3 x1_r = origin;
                    const float3 x1_q = SampleWorldPosition(xyFull, int2(pk_ScreenSize.xy), s_depth);
                    const float3 x2_q = reservoir_q.selected.position;
                    const float3 n2_q = reservoir_q.selected.normal;
                    float3 phi_r_d = x2_q - x1_r;
                    float3 phi_q_d = x2_q - x1_q;
                    float phi_r_l = max(length(phi_r_d), 0.001f);
                    float phi_q_l = max(length(phi_q_d), 0.001f);
                    phi_r_d /= phi_r_l;
                    phi_q_d /= phi_q_l;
                    oneOverJacobian *= safePositiveRcp(getGeometryFactorClamped(n2_q, phi_r_d, phi_r_l));
                    oneOverJacobian *= getGeometryFactorClamped(n2_q, phi_q_d, phi_q_l);
                    oneOverJacobian = saturate(oneOverJacobian);
                }
    
                float targetPdf_curSurf = dot(pk_Luminance.xyz, reservoir_q.selected.radiance);// *oneOverJacobian;
    
                UpdateCombinedReservoir_NewSurf(combined, reservoir_q, targetPdf_curSurf, rnd);
    
                if (targetPdf_curSurf > 0.0)
                {
                    nobiasM += reservoir_q.M;
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