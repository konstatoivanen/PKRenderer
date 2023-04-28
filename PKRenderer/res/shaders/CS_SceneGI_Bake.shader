#version 460
#pragma PROGRAM_COMPUTE
#include includes/Lighting.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

struct SampleIndirect
{
    SH sh;
    float3 radiance;
    float3 direction;
    float luminance;
};

float3 SampleRadiance(const int2 coord, const float3 origin, const float3 direction, const float dist, const float roughness)
{
    const float3 worldpos = origin + direction * dist;
    float3 clipuvw;

    // Try sample previous forward output for better sampling.
    if (Test_WorldToPrevClipUVW(worldpos, clipuvw))
    {
        float sdepth = SamplePreviousLinearDepth(clipuvw.xy);
        bool isMiss = sdepth > pk_ProjectionParams.z - 1e-4f && dist >= PK_GI_RAY_MAX_DISTANCE - 0.01f;
        
        float rdepth = LinearizeDepth(clipuvw.z);
        float sviewz = -SamplePreviousViewNormal(clipuvw.xy).z + 0.15f;
        bool isDepthValid = abs(sdepth - rdepth) < (rdepth * 0.01f / sviewz);

        float2 deltacoord = abs(coord - (clipuvw.xy * pk_ScreenParams.xy));
        bool isCoordValid = dot(deltacoord, deltacoord) > 2.0f;

        if (isCoordValid && (isMiss || isDepthValid))
        {
            return tex2D(pk_ScreenColorPrevious, clipuvw.xy).rgb;
        }
    }

    const float level = roughness * roughness * log2(max(1.0f, dist) / PK_GI_VOXEL_SIZE);
    const float4 voxel = SampleGI_WS(worldpos, level);

    const float3 env = SampleEnvironment(OctaUV(direction), roughness);
    const float envclip = saturate(PK_GI_RAY_MAX_DISTANCE * (1.0f - (dist / PK_GI_RAY_MAX_DISTANCE)));
    const float alpha = max(voxel.a, 1.0f / (PK_GI_VOXEL_MAX_MIP * PK_GI_VOXEL_MAX_MIP));

    return lerp(env, voxel.rgb / alpha, envclip);
}

SampleIndirect GetSample(const int2 coord, const int layer, const float3 normal)
{
    SampleIndirect s;
    s.sh = SampleGI_SH(coord, layer);
    s.direction = normalize(s.sh.Y.wyz + normal * 0.01f);
    s.luminance = SHToLuminance(s.sh, normal);
    return s;
}

SampleIndirect GetSampleNew(const int2 coord, const float3 O, const float3 N, const float3 D, const float hitDistance, const float vxconeSize)
{
    SampleIndirect s;
    s.radiance = SampleRadiance(coord, O, D, hitDistance, vxconeSize);
    s.sh = IrradianceToSH(s.radiance, D);
    s.direction = D;
    s.luminance = SHToLuminance(s.sh, N);
    return s;

}

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 size = int2(pk_ScreenSize.xy);
    int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    const float depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    SceneGIMeta meta = SampleGI_Meta(coord);
    const float4 NR = SampleWorldNormalRoughness(coord);
    const float3 N = NR.xyz;
    const float3 O = SampleWorldPosition(coord, size, depth);
    const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
    const float2 Xi = GetSampleOffset(coord, pk_FrameIndex);
    const float2 hitDist = imageLoad(pk_ScreenGI_Hits, coord).xy;
    
    SampleIndirect diff = GetSample(coord, PK_GI_DIFF_LVL, N);
    SampleIndirect spec = GetSample(coord, PK_GI_SPEC_LVL, N);
    SampleIndirect sDiff = GetSampleNew(coord, O, N, ImportanceSampleLambert(Xi, N), hitDist.x, 0.5f);
    SampleIndirect sSpec = GetSampleNew(coord, O, N, ImportanceSampleSmithGGX(Xi, N, V, NR.w), hitDist.y, 0.0f);

    const float sPDF = max(0.01f, dot(N, sDiff.direction));
    const float sSHPDF = sPDF * pk_L1Basis_Cosine.y * pk_L1Basis.y + pk_L1Basis_Cosine.x * pk_L1Basis.x;
    const float sLum = dot(pk_Luminance.xyz, sDiff.radiance * sSHPDF);

    const float cLum = SHToLuminance(diff.sh, sDiff.direction);
    const float maxLum = max(diff.luminance, sDiff.luminance);
    const float graLum = max(0.0, cLum - sLum) * (1.0 - smoothstep(1.0, 10.0, diff.luminance));

    // @TODO luminance delta scaling based on sh projection is not very accurate. test other alternatives
    const float lumDelta = maxLum > 1e-4f ? saturate(graLum  / maxLum) : 0.0f;
    const float antiLagMult = 1.0f;// pow(1.0 - lumDelta * 0.005f, 10);

    float history = meta.history;
    history *= antiLagMult;
    history = clamp(history + 1.0f, 1.0f, PK_GI_MAX_HISTORY + 1.0f);

    const float wHistory = 1.0f / history;
    const float wDiff = lerp(max(0.01f, wHistory), 1.0f, 0.0f);
    const float wSpec = max(exp(-NR.w) * 0.1f, wHistory);

    diff.sh = InterpolateSH(diff.sh, sDiff.sh, wDiff);
    spec.sh = InterpolateSH(spec.sh, sSpec.sh, wSpec);

    meta.moments = lerp(meta.moments, float2(sDiff.luminance, pow2(sDiff.luminance)), wDiff);
    meta.moments /= lerp(1.0f, 0.0625f, 1.0f / history);
    meta.moments.x = meta.history == 0u ? 0.0f : meta.moments.x;

    StoreGI_Meta(coord, meta);
    StoreGI_SH(coord, PK_GI_DIFF_LVL, diff.sh);
    StoreGI_SH(coord, PK_GI_SPEC_LVL, spec.sh);
}