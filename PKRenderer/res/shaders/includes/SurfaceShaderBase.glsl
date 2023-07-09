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
    float alpha;
    float metallic;     
    float roughness;
    float occlusion;
    float subsurface_distortion;
    float subsurface_power;
    float subsurface_thickness;
};

float3x3 ComposeMikkTangentSpaceMatrix(float3 normal, float4 tangent)
{
    float3 T = normalize(tangent.xyz);
    float3 B = normalize(tangent.w * cross(normal, tangent.xyz));
    float3 N = normalize(normal);
    return mul(float3x3(pk_MATRIX_M), float3x3(T, B, N));
}

float3 SampleNormalTex(in sampler2D map, in float3x3 rotation, in float2 uv, float amount) 
{   
    const float3 n = tex2D(map, uv).xyz * 2.0f - 1.0f;
    return normalize(mul(rotation, lerp(float3(0,0,1), n, amount))); 
}

float2 ParallaxOffset(float height, float heightAmount, float3 viewdir) 
{ 
    return (height * heightAmount - heightAmount / 2.0f) * (viewdir.xy / (viewdir.z + 0.42f)); 
}

float3 GetIndirectLight_Main(const BRDFSurf surf, const float3 worldpos, const float3 clipuvw)
{
    float3 diffuse, specular;
    //diffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
    //specular = SampleEnvironment(OctaUV(reflect(-surf.viewdir, surf.normal)), surf.roughness);
    GI_Sample_Lighting(clipuvw.xy, surf.normal, surf.viewdir, surf.roughness, diffuse, specular);
    return BRDF_INDIRECT_DEFAULT(surf, diffuse, specular);
}

// Multi bounce gi. Causes some very lingering light artifacts & bleeding. @TODO Consider adding a setting for this.
float3 GetIndirectLight_VXGI(const BRDFSurf surf,const float3 worldpos, const float3 clipuvw)
{
    // Get unquantized clip uvw.
    float deltaDepth = SampleViewDepth(clipuvw.xy) - ViewDepth(clipuvw.z); 
    
    // Fragment is in view
    if (deltaDepth > -0.01f && deltaDepth < 0.1f && !GI_Test_VX_History(clipuvw.xy))
    {
        // Sample screen space SH values for more accurate results.
        return surf.albedo * GI_Sample_Diffuse(clipuvw.xy, surf.normal);
    }
    else
    {
        float3 environmentDiffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
        float4 tracedDiffuse = GI_ConeTrace_Diffuse(worldpos, surf.normal, 0.0f);
        return surf.albedo * (environmentDiffuse * tracedDiffuse.a + tracedDiffuse.rgb);
    }
}

// Meta pass specific parameters (gi voxelization requires some changes from reqular view projection).
#if !defined(PK_SURF_BRDF_MAIN)
    #define PK_SURF_BRDF_MAIN BRDF_DEFAULT
#endif

#if !defined(PK_SURF_BRDF_VXGI)
    #define PK_SURF_BRDF_VXGI BRDF_VXGI_DEFAULT
#endif

#define SRC_METALLIC x
#define SRC_OCCLUSION y
#define SRC_ROUGHNESS z

#ZTest Equal
#ZWrite False
#Cull Back
#multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#if defined(PK_META_PASS_GIVOXELIZE) 
    #undef PK_NORMALMAPS
    #undef PK_HEIGHTMAPS
    // Prefilter by using a higher mip bias in voxelization.
    #define PK_SURF_TEX(t, uv) tex2D(t, uv, 4.0f)
    #define PK_META_DECLARE_SURFACE_OUTPUT
    #define PK_META_STORE_SURFACE_OUTPUT(color, worldpos) GI_Store_Voxel(worldpos, color)
    #define PK_META_WORLD_TO_CLIPSPACE(position)  GI_WorldToVoxelNDCSpace(position)
    #define PK_META_BRDF PK_SURF_BRDF_VXGI
    #define PK_META_BRDF_INDIRECT GetIndirectLight_VXGI
#else
    #define PK_SURF_TEX(t, uv) tex2D(t, uv)
    #define PK_META_EARLY_CLIP_UVW(w, c, n) c = GetFragmentClipUVW(); 
    #define PK_META_DECLARE_SURFACE_OUTPUT out float4 SV_Target0;
    #define PK_META_STORE_SURFACE_OUTPUT(color, worldpos) SV_Target0 = color
    #define PK_META_WORLD_TO_CLIPSPACE(position) WorldToClipPos(position)
    #define PK_META_BRDF PK_SURF_BRDF_MAIN
    #define PK_META_BRDF_INDIRECT GetIndirectLight_Main
#endif

struct SurfaceFragmentVaryings
{
    float2 vs_TEXCOORD0;
    float3 vs_WORLDPOSITION;
    #if defined(PK_NORMALMAPS)
        float3x3 vs_TSROTATION;
    #else
        float3 vs_NORMAL;
    #endif
    #if defined(PK_HEIGHTMAPS)
        float3 vs_TSVIEWDIRECTION;
    #endif
};

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
    
        #if defined(PK_NORMALMAPS) || defined(PK_HEIGHTMAPS)
            float3x3 TBN = ComposeMikkTangentSpaceMatrix(in_NORMAL, in_TANGENT);
    
            #if defined(PK_NORMALMAPS)
                baseVaryings.vs_TSROTATION = TBN;
            #endif
    
            #if defined(PK_HEIGHTMAPS)
                baseVaryings.vs_TSVIEWDIRECTION = mul(transpose(TBN), normalize(pk_WorldSpaceCameraPos.xyz - baseVaryings.vs_WORLDPOSITION));
            #endif
        #endif
    
        #if !defined(PK_NORMALMAPS)
            baseVaryings.vs_NORMAL = normalize(ObjectToWorldDir(in_NORMAL.xyz));
        #endif

        PK_SURFACE_FUNC_VERT(baseVaryings);

        gl_Position = PK_META_WORLD_TO_CLIPSPACE(baseVaryings.vs_WORLDPOSITION);
        NORMALIZE_GL_Z;
    }

#elif defined(SHADER_STAGE_FRAGMENT)

    #if defined(PK_HEIGHTMAPS)
        #define PK_SURF_SAMPLE_PARALLAX_OFFSET(heightmap, amount) ParallaxOffset(tex2D(heightmap, varyings.vs_TEXCOORD0.xy).x, amount, normalize(baseVaryings.vs_TSVIEWDIRECTION));
    #else
        #define PK_SURF_SAMPLE_PARALLAX_OFFSET(heightmap, amount) 0.0f.xx 
    #endif

    #if defined(PK_NORMALMAPS)
         #define PK_SURF_SAMPLE_NORMAL(normalmap, amount, uv) SampleNormalTex(normalmap, baseVaryings.vs_TSROTATION, uv, amount)
         #define PK_SURF_MESH_NORMAL normalize(baseVaryings.vs_TSROTATION[2])
    #else
         #define PK_SURF_SAMPLE_NORMAL(normalmap, amount, uv) baseVaryings.vs_NORMAL
         #define PK_SURF_MESH_NORMAL normalize(baseVaryings.vs_NORMAL)
    #endif

    // Use these to modify surface values in fragment stage
    void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf);

    in SurfaceFragmentVaryings baseVaryings;
    PK_META_DECLARE_SURFACE_OUTPUT
    void main()
    {
        float4 value = 0.0f.xxxx;
        
        SurfaceData surf; 
        surf.worldpos = baseVaryings.vs_WORLDPOSITION;
        surf.viewdir = normalize(pk_WorldSpaceCameraPos.xyz - surf.worldpos);
        surf.albedo = 1.0f.xxx;
        surf.normal = float3(0,1,0);
        surf.emission = 0.0f.xxx;
        surf.sheen = 0.0f.xxx;
        surf.alpha = 1.0f;
        surf.metallic = 0.0f;     
        surf.roughness = 1.0f;
        surf.occlusion = 0.0f;
        surf.subsurface_distortion = 0.0f;
        surf.subsurface_power = 1.0f;
        surf.subsurface_thickness = 0.0f;

        #if defined(PK_META_PASS_GIVOXELIZE)
            float3 voxelPos = GI_QuantizeWorldToVoxelSpace(surf.worldpos);

            if (!Test_WorldToClipUVW(voxelPos, surf.clipuvw) || GI_Test_VX_HasValue(surf.worldpos) || !GI_Test_VX_Normal(PK_SURF_MESH_NORMAL))                 
            {                                           
                return;                                 
            }       
            
            surf.clipuvw = WorldToClipUVW(surf.worldpos);                      
            surf.clipuvw.xy = ClampUVScreenBorder(surf.clipuvw.xy);             
        #else
            surf.clipuvw = float3(gl_FragCoord.xy * pk_ScreenParams.zw, gl_FragCoord.z);
        #endif

        PK_SURFACE_FUNC_FRAG(baseVaryings, surf);

        #if defined(PK_META_PASS_GBUFFER)

            value = EncodeGBufferN(normalize(WorldToViewDir(surf.normal)), surf.roughness);

        #else

            #if !defined(PK_META_PASS_GIVOXELIZE)
                // Shift invalid normals to view
                float shiftAmount = dot(surf.normal, surf.viewdir);
                surf.normal = shiftAmount < 0.0f ? surf.normal + surf.viewdir * (-shiftAmount + 1e-5f) : surf.normal;
                surf.roughness = max(surf.roughness, 0.002);
            #endif

            float3 F0 = lerp(pk_DielectricSpecular.rgb, surf.albedo, surf.metallic);
            float reflectivity = pk_DielectricSpecular.r + surf.metallic * pk_DielectricSpecular.a;
            surf.albedo *= 1.0f - reflectivity;
            
            #if defined(PK_SURF_TRANSPARENT)
                surf.albedo *= surf.alpha;
                surf.alpha = reflectivity + surf.alpha * (1.0f - reflectivity);
            #endif

            BRDFSurf brdf_surf = MakeBRDFSurf
            (
                surf.albedo, 
                F0, 
                surf.sheen, 
                surf.normal, 
                surf.viewdir, 
                reflectivity, 
                surf.roughness, 
                surf.subsurface_distortion, 
                surf.subsurface_power, 
                surf.subsurface_thickness
            );

            value.rgb = PK_META_BRDF_INDIRECT(brdf_surf, surf.worldpos, surf.clipuvw);

            #if !defined(PK_META_PASS_GIVOXELIZE)
                value.rgb *= surf.occlusion;
            #endif

            LightTile tile = GetLightTile(surf.clipuvw);
            for (uint i = tile.start; i < tile.end; ++i)
            {
                Light light = GetLight(i, surf.worldpos, tile.cascade);
                value.rgb += PK_META_BRDF(brdf_surf, light.direction, light.color, light.shadow);
            }

            value.rgb += surf.emission;
            value.a = surf.alpha; 

        #endif

        PK_META_STORE_SURFACE_OUTPUT(value, surf.worldpos);
    }

#endif