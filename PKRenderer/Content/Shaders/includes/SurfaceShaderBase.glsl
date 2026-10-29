#pragma once

// needs to be declared before lighting & meshlets include.
#define PK_MESHLET_USE_FUNC_CULL 1
#define PK_MESHLET_SAMPLE_VIEW_DEPTH_MIP SampleMaxZ
#define PK_SURF_DEBUG_SCENE_IBL 0
//#define PK_MESHLET_DEBUG_VIEW_MODE PK_MESHLET_DEBUG_VIEW_MODE_TRIANGLES
//#define PK_MESHLET_MAX_LOD_ONLY 1

#if PK_MESHLET_DEBUG_VIEW_MODE > 0 && (defined(PK_META_PASS_GIVOXELIZE) || defined(PK_META_PASS_GBUFFER))
    #undef PK_MESHLET_DEBUG_VIEW_MODE
#endif

#if defined(PK_META_PASS_GIVOXELIZE) 
    #define SHADOW_TEST ShadowTest_PCF2x2
    #define SHADOW_SAMPLE_SCREENSPACE 0
    #define PK_MESHLET_USE_FUNC_TRIANGLE 1
    // Very high error threshold for voxelize.
    #define PK_MESHLET_LOD_SCALE 8.0f
    #undef SURF_USE_TANGENTS
#else
    #define PK_MESHLET_LOD_SCALE 4.0f
#endif

struct SurfaceVertexUserData
{
    float3 vs_NORMAL : NORMAL0;
    float2 vs_TEXCOORD0 : TEXCOORD0;
    #if defined(SURF_USE_TANGENTS) && !defined(PK_USE_DERIVATIVE_TANGENTS)
    float4 vs_TANGENT : TANGENT0;
    #endif
};

#define PK_MESHLET_VERTEX_USER_DATA SurfaceVertexUserData

#include "Common.glsl"
#include "GBuffers.glsl"
#include "Meshlets.glsl"
#include "BRDF.glsl"
#include "LightSampling.glsl"
#include "SceneEnv.glsl"
#include "SceneGIVX.glsl"

// Meta pass specific parameters (gi voxelization requires some changes from reqular view projection).
#define SRC_METALLIC x
#define SRC_OCCLUSION y
#define SRC_ROUGHNESS z

#pragma pk_ztest Equal
#pragma pk_zwrite False
#pragma pk_cull Back
#pragma pk_multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE
#pragma pk_program Fragment SurfaceShaderMainFs

#if defined(PK_META_PASS_GIVOXELIZE)
    // Prefilter by using a higher mip bias in voxelization.
    #define SURF_DECLARE_RASTER_OUTPUT
    #define SURF_STORE_OUTPUT(value0, value1, world_pos) GI_VX_Store(world_pos, value0);
    #define SURF_TEX(t, uv) t.SampleBias(pk_SamplerTrilinearRepeatAniso, uv, 4.0f)
    #define SURF_TEX_TRIPLANAR(t, n, uvw) SampleTexTriplanar(t, n, uvw, 4.0f)
    #define SURF_WORLD_TO_CLIPSPACE(position) GI_VX_WorldToNdc(position)
    #define SURF_EVALUATE_BxDF BxDF_FullyRoughMinimal
    #define SURF_EVALUATE_BxDF_INDIRECT GetIndirectLight_VXGI
    #define SURF_FS_ASSIGN_WORLDPOSITION(vout) vs_WORLDPOSITION = GI_VX_FragToWorld(vout.sv_Position.xyz);
#else
    #if defined(PK_META_PASS_GBUFFER)
        #define SURF_DECLARE_RASTER_OUTPUT , out float4 SV_Target0 : SV_Target0, out float SV_Target1 : SV_Target1
        #define SURF_STORE_OUTPUT(value0, value1, world_pos) SV_Target0 = value0; SV_Target1 = value1;
    #else
        #define SURF_DECLARE_RASTER_OUTPUT , out float4 SV_Target0 : SV_Target0
        #define SURF_STORE_OUTPUT(value0, value1, world_pos) SV_Target0 = value0;
    #endif
    #define SURF_TEX(t, uv) t.Sample(pk_SamplerTrilinearRepeatAniso, uv)
    #define SURF_TEX_TRIPLANAR(t, n, uvw) SampleTexTriplanar(t, n, uvw, 0.0f)
    #define SURF_WORLD_TO_CLIPSPACE(position) WorldToClipPos(position)
    #define SURF_EVALUATE_BxDF BxDF_Principled
    #define SURF_EVALUATE_BxDF_INDIRECT GetIndirectLight_Main
    #define SURF_FS_ASSIGN_WORLDPOSITION(vout) vs_WORLDPOSITION = UvToWorldPos(vout.sv_Position.xy * pk_View.ScreenParams.zw, ViewDepth(vout.sv_Position.z));
#endif

#if defined(SURF_USE_TANGENTS)
    #define SURF_MESH_NORMAL vs_TANGENTSPACE[2]
    #define SURF_MAKE_PARALLAX_OFFSET(height, amount, view_dir) CalculateParallaxUvOffset(vs_TANGENTSPACE, height, amount, view_dir)
    #define SURF_SAMPLE_NORMAL(normalmap, amount, uv) SampleNormalTex(normalmap, vs_TANGENTSPACE, uv, amount)
    #define SURF_SAMPLE_NORMAL_TRIPLANAR(normalmap, amount, uvw) SampleNormalTexTriplanar(normalmap, vs_TANGENTSPACE, uvw, amount)
    #if defined(PK_USE_DERIVATIVE_TANGENTS)
        #define SURF_VS_ASSIGN_TANGENT(userdata, value)
        #define SURF_FS_ASSIGN_TBN(userdata) vs_TANGENTSPACE = half3x3(ComposeDerivativeTBN(userdata.vs_NORMAL, vs_WORLDPOSITION, userdata.vs_TEXCOORD0));
    #else
        #define SURF_VS_ASSIGN_TANGENT(userdata, value) userdata.vs_TANGENT = value;
        #define SURF_FS_ASSIGN_TBN(userdata) vs_TANGENTSPACE = half3x3(ComposeMikkTBN(userdata.vs_NORMAL, userdata.vs_TANGENT));
    #endif
#else
    #define SURF_VS_ASSIGN_TANGENT(userdata, value)
    #define SURF_FS_ASSIGN_TBN(userdata) vs_NORMAL = userdata.vs_NORMAL;
    #define SURF_MESH_NORMAL normalize(vs_NORMAL)
    #define SURF_MAKE_PARALLAX_OFFSET(height, amount, view_dir) 0.0f.xx 
    #define SURF_SAMPLE_NORMAL(normalmap, amount, uv) SURF_MESH_NORMAL
    #define SURF_SAMPLE_NORMAL_TRIPLANAR(normalmap, amount, uvw) SURF_MESH_NORMAL
#endif

struct SurfaceVaryings
{
    float3 world_pos;
    float3 normal;
    float4 tangent;
    float2 texcoord;
};

struct SurfaceData
{
    float3 view_dir;
    float3 world_pos;
    float3 clip_uvw;
    float3 albedo;
    float3 normal;
    float3 emission;
    float3 sheen;
    float3 subsurface;
    float sheen_roughness;
    float clear_coat;
    float clear_coat_gloss;
    float alpha;
    float metallic;     
    float roughness;
    float occlusion;
    float depth_bias;
};

//// ---------- MESH TASK STAGE ---------- ////
bool PK_MESHLET_FUNC_CULL(PKMeshTaskPayload payload, const PKMeshlet meshlet)
{
    #if defined(PK_META_PASS_GIVOXELIZE)
    return true;
    #elif defined(PK_META_PASS_GBUFFER)
    return Meshlet_Cull_Cone(meshlet, pk_View.ViewWorldOrigin.xyz) && 
           Meshlet_Cull_Frustum_Perspective(meshlet, pk_View.WorldToClip);
    #else
    return Meshlet_Cull_Cone(meshlet, pk_View.ViewWorldOrigin.xyz) && 
           Meshlet_Cull_Depth_Perspective(meshlet, pk_View.WorldToClip, pk_View.ViewToClip, int2(pk_View.ScreenSize), pk_View.ScreenLevels);
    #endif
}

//// ---------- MESH ASSEMBLY STAGE ---------- ////
#if defined(SURF_USE_VERTEX_FUNCTION)
void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf);
#else
void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf) {}
#endif
    
#if defined(PK_META_PASS_GIVOXELIZE)
groupshared float3 lds_Positions[MAX_VERTICES_PER_MESHLET];

// Cull triangles that should be rasterized through another axis
void PK_MESHLET_FUNC_TRIANGLE(uint triangle_index, PKMeshTaskPayload payload, inout uint3 triangle, inout PKMeshPrimitiveOut pout)
{
    const float3 a = lds_Positions[triangle.x];
    const float3 b = normalize(lds_Positions[triangle.y] - a);
    const float3 c = normalize(lds_Positions[triangle.z] - a);
    const float3 n = normalize(cross(b, c));
    pout.sv_CullPrimitive = !GI_VX_Test_Normal(n);
}
#endif

void PK_MESHLET_FUNC_VERTEX(uint vertex_index, PKMeshTaskPayload payload, PKVertex vertex, inout PKMeshletVertexVaryings outVaryings)
{
    SurfaceVaryings varyings = SurfaceVaryings
    (
        ObjectToWorldPos(vertex.position.xyz),
        ObjectToWorldVec(vertex.normal),
        float4(ObjectToWorldVec(vertex.tangent.xyz), vertex.tangent.w),
        vertex.texcoord
    );

    SURF_FUNCTION_VERTEX(varyings);

    #if defined(PK_META_PASS_GIVOXELIZE)
    lds_Positions[vertex_index] = varyings.world_pos;
    #endif

    outVaryings.sv_Position = SURF_WORLD_TO_CLIPSPACE(varyings.world_pos);
    outVaryings.userdata.vs_NORMAL = varyings.normal;
    outVaryings.userdata.vs_TEXCOORD0 = varyings.texcoord;
    SURF_VS_ASSIGN_TANGENT(outVaryings.userdata, varyings.tangent)
}

//// ---------- FRAGMENT STAGE ---------- ////
float2 CalculateParallaxUvOffset(const half3x3 tangentSpace, float height, float amount, float3 view_dir) 
{ 
    view_dir = mul(float3x3(tangentSpace), view_dir);
    return (height * amount - amount * 0.5f) * view_dir.xy / (view_dir.z + 0.5f); 
}

// http://www.thetenthplanet.de/archives/1180
float3x3 ComposeDerivativeTBN(float3 normal, const float3 position, const float2 texcoord)
{
    normal = normalize(normal);
    const float3 dp1 = ddx_fine(position);
    const float3 dp2 = ddy_fine(position);
    const float2 duv1 = ddx_fine(texcoord);
    const float2 duv2 = ddy_fine(texcoord);
    const float3 dp2perp = cross(dp2, normal);
    const float3 dp1perp = cross(normal, dp1);
    const float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    const float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
    const float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    return float3x3(-T * invmax, -B * invmax, normal);
}

float3x3 ComposeMikkTBN(float3 normal, float4 tangent)
{
    const float3 T = normalize(tangent.xyz);
    const float3 N = normalize(normal);
    const float3 B = sign(tangent.w) * cross(N, T);
    return float3x3(T, B, N);
}

float3 SampleNormalTex(const Texture2D map, const float3x3 rotation, const float2 uv, const float amount) 
{
    float3 normal = map.Sample(pk_SamplerTrilinearRepeatAniso, uv).xyz * 2.0f - 1.0f;
    normal = lerp(float3(0, 0, 1), normal, amount);
    return normalize(mul(normal, rotation)); 
}

float3 SampleNormalTexTriplanar(const Texture2D tex, const float3x3 rotation, const float3 uvw, const float amount) 
{
    float3 blend = abs(rotation[2]);
    blend /= dot(blend, 1.0f.xxx);
    const float3 cx = tex.Sample(pk_SamplerTrilinearRepeatAniso, uvw.yz).xyz;
    const float3 cy = tex.Sample(pk_SamplerTrilinearRepeatAniso, uvw.xz).xyz;
    const float3 cz = tex.Sample(pk_SamplerTrilinearRepeatAniso, uvw.xy).xyz;
    float3 normal = (cx * blend.x + cy * blend.y + cz * blend.z) * 2.0f - 1.0f;
    normal = lerp(float3(0, 0, 1), normal, amount);
    return normalize(mul(normal, rotation)); 
}

float4 SampleTexTriplanar(const Texture2D tex, const float3 normal, const float3 uvw, float bias)
{
    float3 blend = abs(normal);
    blend /= dot(blend, 1.0f.xxx);
    const float4 cx = tex.SampleBias(pk_SamplerTrilinearRepeatAniso, uvw.yz, bias);
    const float4 cy = tex.SampleBias(pk_SamplerTrilinearRepeatAniso, uvw.xz, bias);
    const float4 cz = tex.SampleBias(pk_SamplerTrilinearRepeatAniso, uvw.xy, bias);
    return cx * blend.x + cy * blend.y + cz * blend.z;
}
    
float3 GetIndirectLight_Main(const BxDFSurf surf, const float3 world_pos, const float3 clip_uvw)
{
    #if PK_SURF_DEBUG_SCENE_IBL
        const float3 ls_dir = Futil_SpecularDominantDirection(surf.normal, surf.view_dir, sqrt(surf.alpha));
        const float2 ls_uv = EncodeOctaUv(ls_dir);
        const float3 peak_direction = SceneEnv_Sample_SH_PeakDirection();
        const float3 peak_color = SceneEnv_Sample_SH_Color();
        const float3 ld = SceneEnv_Sample_SH_Diffuse(surf.normal);
        const float3 ls = SceneEnv_Sample_IBL(ls_uv, surf.alpha);
        const float ls_fade = 1.0f;
    #else
        const GIResolved resolved = GI_Load_Resolved(clip_uvw.xy); 
        const float3 peak_direction = GI_Evaluate_Peak_Direction(resolved);
        const float3 peak_color = GI_Evaluate_Peak_Color(resolved);
        const float3 ld = GI_Evaluate_Diffuse(resolved, surf.normal);
        const float3 ls = resolved.spec * resolved.spec_ao;

        #if PK_GI_APPROX_ROUGH_SPEC == 1
        const float ls_fade = 1.0f - GI_RoughSpecWeight(sqrt(surf.alpha));
        #else
        const float ls_fade = 1.0f;
        #endif
    #endif

    return BxDF_SceneGI(surf, peak_direction, peak_color, ld, ls, ls_fade);
}
    
float3 GetIndirectLight_VXGI(const BxDFSurf surf, const float3 world_pos, const float3 clip_uvw)
{
    const float depth_view = SampleViewDepth(clip_uvw.xy);
    const float depth_surf = ViewDepth(clip_uvw.z);
    const bool is_invalid_surf = (abs(depth_view - depth_surf) / depth_view) > 0.025f;

    // Fragment is in view
    [[branch]]
    if (QuadAll(is_invalid_surf))
    {
        const float3 diff_scene_env = SceneEnv_Sample_SH_Diffuse(surf.normal);
        // Occlude ground from secondary bounces.
        const float3 diff_scene_env_masked = diff_scene_env * step(0.0f, surf.normal.y);
        const float4 diff_voxel_traced = GI_VX_ConeTrace_Diffuse(world_pos, surf.normal);
        return surf.diffuse * (diff_scene_env_masked * diff_voxel_traced.a + diff_voxel_traced.rgb);
    }
    else
    {
        // Sample screen space SH values for more accurate results.
        return surf.diffuse * GI_Evaluate_Diffuse(GI_Load_Resolved(clip_uvw.xy), surf.normal);
    }
}

// Use these to modify surface values in fragment stage
void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf);

static half3x3 vs_TANGENTSPACE;
static float3 vs_WORLDPOSITION;
static float3 vs_NORMAL;

[earlydepthstencil]
void SurfaceShaderMainFs(PKMeshletVertexVaryings vout SURF_DECLARE_RASTER_OUTPUT)
{
    PK_INSTANCING_ASSIGN_FRAGMENT_LOCALS(vout);
    SURF_FS_ASSIGN_WORLDPOSITION(vout)
    SURF_FS_ASSIGN_TBN(vout.userdata)

    SurfaceData surf = (SurfaceData)0;
    surf.view_dir = normalize(pk_View.ViewWorldOrigin.xyz - vs_WORLDPOSITION);
    surf.world_pos = vs_WORLDPOSITION;
    surf.normal = float3(0,0,1);
    surf.albedo = 1.0f.xxx;
    surf.alpha = 1.0f;
    surf.roughness = 1.0f;
        
    #if defined(PK_META_PASS_GIVOXELIZE)
        surf.clip_uvw = WorldToClipUvw(surf.world_pos);

        [[branch]]
        if (!Test_InUvw(surf.clip_uvw) || GI_VX_Test_HasValue(surf.world_pos))                 
        {
            return;
        }

        surf.clip_uvw.xy = ClampBilinearViewUv(surf.clip_uvw.xy);
        int2 coord_screen = int2(surf.clip_uvw.xy * pk_View.ScreenSize.xy);
    #else
        surf.clip_uvw = float3(vout.sv_Position.xy * pk_View.ScreenParams.zw, vout.sv_Position.z);
        int2 coord_screen = int2(vout.sv_Position.xy);
    #endif

    SURF_FUNCTION_FRAGMENT(vout.userdata.vs_TEXCOORD0, surf);

    #if !defined(PK_META_PASS_GIVOXELIZE)
        const float shift_amount = dot(surf.normal, surf.view_dir);
        surf.normal = shift_amount < 0.0f ? surf.normal + surf.view_dir * (-shift_amount + 1e-5f) : surf.normal;
        surf.roughness = max(surf.roughness, 0.002f);
    #endif

    float4 sv_output0 = 0.0f.xxxx;
    float sv_output1 = 0.0f;

    #if defined(PK_META_PASS_GBUFFER)
        sv_output0 = EncodeGBufferWorldNR(surf.normal, surf.roughness, surf.metallic);
        sv_output1 = EncodeBiasedDepth(surf.clip_uvw.z, dot(surf.view_dir, surf.normal), surf.depth_bias);
    #else

        // Metallic workflow
        float3 F0 = Futil_ComputeF0(surf.albedo, surf.metallic, surf.clear_coat);
        float3 diffuse_color = Futil_ComputeDiffuseColor(surf.albedo, surf.metallic);

        #if defined(SURF_TRANSPARENT)
            diffuse_color = Futil_PremultiplyTransparency(diffuse_color, surf.metallic, /*inout*/ surf.alpha);
        #endif

        BxDFSurf bxdf_surf = BxDFSurf
        (
            diffuse_color,
            F0,
            surf.normal,
            surf.view_dir,
            surf.subsurface,
            surf.sheen,
            pow2(surf.sheen_roughness),
            surf.clear_coat,
            surf.clear_coat_gloss,
            pow2(surf.roughness), // Convert linear roughness to roughness
            max(0.0f, dot(surf.normal, surf.view_dir))
        );

        sv_output0.rgb = SURF_EVALUATE_BxDF_INDIRECT(bxdf_surf, surf.world_pos, surf.clip_uvw);

        #if !defined(PK_META_PASS_GIVOXELIZE)
            sv_output0.rgb *= surf.occlusion;
        #endif

        LightTile tile = Lights_LoadTile_Px(coord_screen, ViewDepth(surf.clip_uvw.z));
        
        for (uint i = tile.start; i < tile.end; ++i)
        {
            SceneLightSample light = Lights_SampleTiled(i, surf.world_pos, surf.normal, tile.cascade);
            sv_output0.rgb += SURF_EVALUATE_BxDF(bxdf_surf, light.Lv, light.Rv, light.shadow, light.source_radius);
        }

        sv_output0.rgb += surf.emission;
        sv_output0.a = surf.alpha;

        PK_MESHLET_DEBUG_VISUALIZE(vout, sv_output0.rgb);

    #endif

    SURF_STORE_OUTPUT(sv_output0, sv_output1, surf.world_pos)
}
