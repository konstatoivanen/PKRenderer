#pragma once
#include Lighting.glsl
#include BRDF.glsl
#include Reconstruction.glsl
#include SharedSceneGI.glsl

// Meta pass specific parameters (gi voxelization requires some changes from reqular view projection).
#ZTest Equal
#ZWrite False
#Cull Back
#multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#if defined(PK_META_PASS_GIVOXELIZE) 
    #undef PK_NORMALMAPS
    #undef PK_HEIGHTMAPS
    
    #define PK_META_EARLY_CLIP_UVW(w, c, n)         \
        float3 vq = QuantizeWorldToVoxelSpace(w);   \
        if (!TryGetWorldToClipUVW(vq, c) ||         \
             SceneGIVoxelHasValue(w) ||             \
             !SceneGINormalReject(n))               \
        {                                           \
            return;                                 \
        }                                           \
        c = WorldToClipUVW(w);                      \
        c.xy = ClampClipUVBorder(c.xy);             \


    #define PK_META_DECLARE_SURFACE_OUTPUT
    #define PK_META_STORE_SURFACE_OUTPUT(color, worldpos) StoreGI_WS(worldpos, color)
    #define PK_META_WORLD_TO_CLIPSPACE(position)  WorldToVoxelNDCSpace(position)
#else
    #define PK_META_EARLY_CLIP_UVW(w, c, n) c = GetFragmentClipUVW(); 
    #define PK_META_DECLARE_SURFACE_OUTPUT out float4 SV_Target0;
    #define PK_META_STORE_SURFACE_OUTPUT(color, worldpos) SV_Target0 = color
    #define PK_META_WORLD_TO_CLIPSPACE(position) WorldToClipPos(position)
#endif

#define SRC_METALLIC x
#define SRC_OCCLUSION y
#define SRC_ROUGHNESS z

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

float3 GetSurfaceSpecularColor(float3 albedo, float metallic) { return lerp(pk_DielectricSpecular.rgb, albedo, metallic); }

float GetSurfaceAlphaReflectivity(inout SurfaceData surf)
{
    float reflectivity = pk_DielectricSpecular.r + surf.metallic * pk_DielectricSpecular.a;
    surf.albedo *= 1.0f - reflectivity;
    
    #if defined(PK_TRANSPARENT_PBR)
        surf.albedo *= surf.alpha;
        surf.alpha = reflectivity + surf.alpha * (1.0f - reflectivity);
    #endif

    return reflectivity;
}

// Fix edge artifacts for when normals are pointing away from camera.
float3 GetViewShiftedNormal(float3 normal, float3 viewdir)
{
    float shiftAmount = dot(normal, viewdir);
    return shiftAmount < 0.0f ? normal + viewdir * (-shiftAmount + 1e-5f) : normal;
}

Indirect GetStaticSceneIndirect(float3 normal, float3 viewdir, float roughness)
{
    Indirect indirect;
    indirect.diffuse = SampleEnvironment(OctaUV(normal), 1.0f);
    indirect.specular = SampleEnvironment(OctaUV(reflect(-viewdir, normal)), roughness);
    return indirect;
}

#if defined(SHADER_STAGE_VERTEX)

    // Use these to modify surface values in fragment or vertex stage
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
            baseVaryings.vs_NORMAL = ObjectToWorldDir(in_NORMAL.xyz);
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
         #define PK_SURF_SAMPLE_NORMAL(normalmap, amount, uv) SampleNormal(normalmap, baseVaryings.vs_TSROTATION, uv, amount)
         #define PK_SURF_MESH_NORMAL normalize(baseVaryings.vs_TSROTATION[2])
    #else
         #define PK_SURF_SAMPLE_NORMAL(normalmap, amount, uv) baseVaryings.vs_NORMAL
         #define PK_SURF_MESH_NORMAL normalize(baseVaryings.vs_NORMAL)
    #endif

    // Use these to modify surface values in fragment or vertex stage
    void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf);

    in SurfaceFragmentVaryings baseVaryings;
    PK_META_DECLARE_SURFACE_OUTPUT
    void main()
    {
        float4 value = 0.0f.xxxx;
        
        // Init defaults
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

        PK_META_EARLY_CLIP_UVW(surf.worldpos, surf.clipuvw, PK_SURF_MESH_NORMAL)
        
        PK_SURFACE_FUNC_FRAG(baseVaryings, surf);

        #if defined(PK_META_PASS_GBUFFER)

            value = float4(WorldToViewDir(surf.normal), surf.roughness);

        #elif defined(PK_META_PASS_GIVOXELIZE)
            GetSurfaceAlphaReflectivity(surf);

            LightTile tile = GetLightTile(surf.clipuvw);
    
            for (uint i = tile.start; i < tile.end; ++i)
            {
                Light light = GetSurfaceLight(i, surf.worldpos, tile.cascade);
                value.rgb += PK_ACTIVE_VXGI_BRDF(surf.albedo, surf.normal, light);
            }
    
            // Multi bounce gi. Causes some very lingering light artifacts & bleeding. @TODO Consider adding a setting for this.
            float3 indirect = 0.0f.xxx;

            // Get unquantized clip uvw.
            float deltaDepth = SampleLinearDepth(surf.clipuvw.xy) - LinearizeDepth(surf.clipuvw.z); 
            
            // Fragment is in view
            if (deltaDepth > -0.01f && deltaDepth < 0.1f)
            {
                // Sample screen space SH values for more accurate results.
                indirect = SampleGI_VS_Diffuse(surf.clipuvw.xy, surf.normal);
            }
            else
            {
                float3 environmentDiffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
                float4 tracedDiffuse = SampleGI_ConeTraceDiffuse(surf.worldpos, surf.normal, 0.0f);
                indirect = (environmentDiffuse * tracedDiffuse.a + tracedDiffuse.rgb);
            }

            value.rgb += surf.albedo * indirect;
            value.rgb += surf.emission;
            value.a = surf.alpha; 

        #else

            surf.roughness = max(surf.roughness, 0.002);
            surf.normal = GetViewShiftedNormal(surf.normal, surf.viewdir);
            float3 specColor = GetSurfaceSpecularColor(surf.albedo, surf.metallic);
            float reflectivity = GetSurfaceAlphaReflectivity(surf);
            Indirect indirect = GetStaticSceneIndirect(surf.normal, surf.viewdir, surf.roughness);
            LightTile tile = GetLightTile(surf.clipuvw);
    
            SampleGI_VS
            (
                indirect.diffuse, 
                indirect.specular, 
                surf.clipuvw.xy, 
                surf.normal, 
                -surf.viewdir, 
                surf.roughness
            );
    
            INIT_BRDF_CACHE
            (
                surf.albedo, 
                specColor, 
                surf.sheen, 
                surf.normal, 
                surf.viewdir, 
                reflectivity, 
                surf.roughness, 
                surf.subsurface_distortion, 
                surf.subsurface_power, 
                surf.subsurface_thickness
            );
    
            value.rgb = BRDF_PBS_DEFAULT_INDIRECT(indirect);
            value.rgb *= surf.occlusion;
    
            for (uint i = tile.start; i < tile.end; ++i)
            {
                Light light = GetSurfaceLight(i, surf.worldpos, tile.cascade);
                value.rgb += PK_ACTIVE_BRDF(light);
            }
    
            value.rgb += surf.emission;
            value.a = surf.alpha;
        #endif

        PK_META_STORE_SURFACE_OUTPUT(value, surf.worldpos);
    }

#endif