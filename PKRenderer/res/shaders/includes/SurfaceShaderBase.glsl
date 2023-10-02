#pragma once
#include GBuffers.glsl
#include Lighting.glsl
#include SceneEnv.glsl
#include SharedSceneGI.glsl

struct SurfaceData
{
    float3 viewdir;
    float3 worldpos;
    float3 clipuvw;
    float3 albedo;      
    float3 normal;      
    float3 emission;
    float3 sheen;
    float3 subsurface;
    float3 clearCoat;
    float sheenTint;
    float clearCoatGloss;
    float alpha;
    float metallic;     
    float roughness;
    float occlusion;
};

float3x3 ComposeMikkTBN(float3 normal, float4 tangent)
{
    float3 T = normalize(tangent.xyz);
    float3 N = normalize(normal);
    float3 B = sign(tangent.w) * cross(N, T);
    return float3x3(T, B, N);
}

float3 SampleNormalTex(in sampler2D map, in float3x3 rotation, in float2 uv, float amount) 
{   
    const float3 n = tex2D(map, uv).xyz * 2.0f - 1.0f;
    return normalize(mul(rotation, lerp(float3(0,0,1), n, amount))); 
}

float2 SampleParallaxOffset(in sampler2D map, in float2 uv, float amount, float3 viewdir) 
{ 
    return (tex2D(map, uv).x * amount - amount * 0.5f) * viewdir.xy / (viewdir.z + 0.5f); 
}

float3 GetIndirectLight_Main(const BxDFSurf surf, const float3 worldpos, const float3 clipuvw)
{
    //float3 diffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
    //float3 specular = SampleEnvironment(OctaUV(reflect(-surf.viewdir, surf.normal)), surf.alpha);
    float3 diffuse = GI_Load_Resolved_Diff(clipuvw.xy);
    float3 specular = GI_Load_Resolved_Spec(clipuvw.xy);
    return EvaluateBxDF_Indirect(surf, diffuse, specular);
}

// Multi bounce gi. Causes some very lingering light artifacts & bleeding. @TODO Consider adding a setting for this.
float3 GetIndirectLight_VXGI(const BxDFSurf surf, const float3 worldpos, const float3 clipuvw)
{
    // Get unquantized clip uvw.
    float deltaDepth = SampleViewDepth(clipuvw.xy) - ViewDepth(clipuvw.z); 
    
    // Fragment is in view
    if (deltaDepth > -0.01f && deltaDepth < 0.1f)
    {
        // Sample screen space SH values for more accurate results.
        return surf.albedo * GI_Load_Resolved_Diff(clipuvw.xy);
    }
    else
    {
        float3 environmentDiffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
        float4 tracedDiffuse = GI_ConeTrace_Diffuse(worldpos, surf.normal, 0.0f);
        return surf.albedo * (environmentDiffuse * tracedDiffuse.a + tracedDiffuse.rgb);
    }
}

// Meta pass specific parameters (gi voxelization requires some changes from reqular view projection).
#define SRC_METALLIC x
#define SRC_OCCLUSION y
#define SRC_ROUGHNESS z

#ZTest Equal
#ZWrite False
#Cull Back
#multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#if defined(PK_META_PASS_GIVOXELIZE) 
    #undef PK_USE_TANGENTS
    // Prefilter by using a higher mip bias in voxelization.
    #define PK_SURF_TEX(t, uv) tex2D(t, uv, 4.0f)
    #define PK_META_DECLARE_SURFACE_OUTPUT
    #define PK_META_STORE_SURFACE_OUTPUT(color, worldpos) GI_Store_Voxel(worldpos, color)
    #define PK_META_WORLD_TO_CLIPSPACE(position)  GI_WorldToVoxelNDCSpace(position)
    #define PK_META_BxDF EvaluateBxDF_DirectMinimal
    #define PK_META_BxDF_INDIRECT GetIndirectLight_VXGI
#else
    #define PK_SURF_TEX(t, uv) tex2D(t, uv)
    #define PK_META_DECLARE_SURFACE_OUTPUT out float4 SV_Target0;
    #define PK_META_STORE_SURFACE_OUTPUT(color, worldpos) SV_Target0 = color
    #define PK_META_WORLD_TO_CLIPSPACE(position) WorldToClipPos(position)
    #define PK_META_BxDF EvaluateBxDF_Direct
    #define PK_META_BxDF_INDIRECT GetIndirectLight_Main
#endif

struct SurfaceFragmentVaryings
{
    float3 vs_WORLDPOSITION;
    float3 vs_NORMAL;
    #if defined(PK_USE_TANGENTS)
    float4 vs_TANGENT;
    #endif
    float2 vs_TEXCOORD0;
};

//// ---------- VERTEX STAGE ---------- ////
#if defined(SHADER_STAGE_VERTEX)

    // Use these to modify surface values in vertex stage
    void PK_SURFACE_FUNC_VERT(inout SurfaceFragmentVaryings surf);

    in float3 in_POSITION;
    in float3 in_NORMAL;
    in float4 in_TANGENT;
    in float2 in_TEXCOORD0;
    out SurfaceFragmentVaryings baseVaryings;
    
    void main()
    {
        baseVaryings.vs_WORLDPOSITION = ObjectToWorldPos(in_POSITION.xyz);
        baseVaryings.vs_TEXCOORD0 = in_TEXCOORD0;
        baseVaryings.vs_NORMAL = ObjectToWorldDir(in_NORMAL);
    #if defined(PK_USE_TANGENTS)
        baseVaryings.vs_TANGENT.xyz = ObjectToWorldDir(in_TANGENT.xyz);
        baseVaryings.vs_TANGENT.w = in_TANGENT.w;
    #endif
        PK_SURFACE_FUNC_VERT(baseVaryings);
        gl_Position = PK_META_WORLD_TO_CLIPSPACE(baseVaryings.vs_WORLDPOSITION);
    }

//// ---------- FRAGMENT STAGE ---------- ////
#elif defined(SHADER_STAGE_FRAGMENT)

    #if defined(PK_USE_TANGENTS)
        half3x3 pk_MATRIX_TBN;
        #define PK_SURF_MESH_NORMAL pk_MATRIX_TBN[2]
        #define PK_SURF_SAMPLE_PARALLAX_OFFSET(heightmap, amount, uv, viewdir) SampleParallaxOffset(heightmap, uv, amount, mul(transpose(pk_MATRIX_TBN), viewdir))
        #define PK_SURF_SAMPLE_NORMAL(normalmap, amount, uv) SampleNormalTex(normalmap, pk_MATRIX_TBN, uv, amount)
    #else
        #define PK_SURF_MESH_NORMAL normalize(baseVaryings.vs_NORMAL)
        #define PK_SURF_SAMPLE_PARALLAX_OFFSET(heightmap, amount, uv, viewdir) 0.0f.xx 
        #define PK_SURF_SAMPLE_NORMAL(normalmap, amount, uv) PK_SURF_MESH_NORMAL
    #endif

    // Use these to modify surface values in fragment stage
    void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf);

    in SurfaceFragmentVaryings baseVaryings;
    PK_META_DECLARE_SURFACE_OUTPUT

    void main()
    {
        #if defined(PK_USE_TANGENTS)
        pk_MATRIX_TBN = half3x3(ComposeMikkTBN(baseVaryings.vs_NORMAL, baseVaryings.vs_TANGENT));
        #endif

        float4 sv_output = 0.0f.xxxx;

        SurfaceData surf; 
        surf.worldpos = baseVaryings.vs_WORLDPOSITION;
        surf.viewdir = normalize(pk_WorldSpaceCameraPos.xyz - surf.worldpos);
        surf.albedo = 1.0f.xxx;
        surf.normal = float3(0,1,0);
        surf.emission = 0.0f.xxx;
        surf.sheen = 0.0f.xxx;
        surf.subsurface = 0.0f.xxx;
        surf.clearCoat = 0.0f.xxx;
        surf.sheenTint = 0.0f;
        surf.clearCoatGloss = 0.0f;
        surf.alpha = 1.0f;
        surf.metallic = 0.0f;     
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        
        // Screen space pixel coord
        int2 screencoord = int2(gl_FragCoord.xy);

    #if defined(PK_META_PASS_GIVOXELIZE)
        float3 voxelPos = GI_QuantizeWorldToVoxelSpace(surf.worldpos);

        if (!Test_WorldToClipUVW(voxelPos, surf.clipuvw) || GI_Test_VX_HasValue(surf.worldpos) || !GI_Test_VX_Normal(PK_SURF_MESH_NORMAL))                 
        {                                           
            return;                                 
        }       
        
        surf.clipuvw = WorldToClipUVW(surf.worldpos);                      
        surf.clipuvw.xy = ClampUVScreenBorder(surf.clipuvw.xy);          
        screencoord = int2(surf.clipuvw.xy * pk_ScreenSize.xy);
    #else
        surf.clipuvw = float3(gl_FragCoord.xy * pk_ScreenParams.zw, gl_FragCoord.z);
    #endif

        PK_SURFACE_FUNC_FRAG(baseVaryings, surf);

    #if !defined(PK_META_PASS_GIVOXELIZE)
        const float shiftAmount = dot(surf.normal, surf.viewdir);
        surf.normal = shiftAmount < 0.0f ? surf.normal + surf.viewdir * (-shiftAmount + 1e-5f) : surf.normal;
        surf.roughness = max(surf.roughness, 0.002);
    #endif

    #if defined(PK_META_PASS_GBUFFER)
        sv_output = EncodeGBufferWorldNR(surf.normal, surf.roughness);
    #else

        const float3 F0 = lerp(pk_DielectricSpecular.rgb, surf.albedo, surf.metallic);
        const float reflectivity = pk_DielectricSpecular.r + surf.metallic * pk_DielectricSpecular.a;
        surf.albedo *= 1.0f - reflectivity;

        #if defined(PK_SURF_TRANSPARENT)
        surf.albedo *= surf.alpha;
        surf.alpha = reflectivity + surf.alpha * (1.0f - reflectivity);
        #endif

        BxDFSurf bxdf_surf = BxDFSurf
        (
            surf.albedo, 
            F0, 
            surf.normal, 
            surf.viewdir, 
            surf.sheen, 
            surf.subsurface, 
            surf.clearCoat, 
            surf.sheenTint,
            surf.clearCoatGloss,
            reflectivity, 
            pow2(surf.roughness), // Convert linear roughness to roughness
            max(0.0f, dot(surf.normal, surf.viewdir))
        );

        sv_output.rgb = PK_META_BxDF_INDIRECT(bxdf_surf, surf.worldpos, surf.clipuvw);

        #if !defined(PK_META_PASS_GIVOXELIZE)
        sv_output.rgb *= surf.occlusion;
        #endif

        LightTile tile = GetLightTile_PX(screencoord, ViewDepth(surf.clipuvw.z));
        for (uint i = tile.start; i < tile.end; ++i)
        {
            Light light = GetLight(i, surf.worldpos, tile.cascade);
            sv_output.rgb += PK_META_BxDF(bxdf_surf, light.direction, light.color, light.shadow, light.sourceRadius);
        }

        sv_output.rgb += surf.emission;
        sv_output.a = surf.alpha; 
    #endif

        PK_META_STORE_SURFACE_OUTPUT(sv_output, surf.worldpos);
    }

#endif