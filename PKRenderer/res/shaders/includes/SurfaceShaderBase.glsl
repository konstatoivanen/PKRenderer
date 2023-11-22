#pragma once

// needs to be declared before lighting & meshlets include.
#define PK_MESHLET_USE_FRUSTUM_CULL 1
#define PK_MESHLET_USE_FUNC_CULL 1
#define PK_MESHLET_USE_FUNC_TASKLET 1

#if defined(PK_META_PASS_GIVOXELIZE) 
    #define SHADOW_TEST ShadowTest_PCF2x2
    #define SHADOW_SAMPLE_SCREENSPACE 0
    #define PK_MESHLET_USE_FUNC_TRIANGLE 1
#endif

#include Common.glsl
#if !defined(SHADER_STAGE_MESH_TASK)
#include GBuffers.glsl
#include Lighting.glsl
#include SceneEnv.glsl
#include SceneGIVX.glsl
#endif
#include Meshlets.glsl

// Meta pass specific parameters (gi voxelization requires some changes from reqular view projection).
#define SRC_METALLIC x
#define SRC_OCCLUSION y
#define SRC_ROUGHNESS z

#ZTest Equal
#ZWrite False
#Cull Back
#multi_compile _ PK_META_PASS_GBUFFER PK_META_PASS_GIVOXELIZE

#if defined(PK_META_PASS_GIVOXELIZE) 
    #undef SURF_USE_TANGENTS
    // Prefilter by using a higher mip bias in voxelization.
    #define SURF_DECLARE_RASTER_OUTPUT
    #define SURF_STORE_OUTPUT(value0, value1, worldpos) GI_Store_Voxel(worldpos, value0);
    #define SURF_TEX(t, uv) texture(sampler2D(t, pk_Sampler_SurfDefault), uv, 4.0f)
    #define SURF_TEX_TRIPLANAR(t, n, uvw) SampleTexTriplanar(t,n,uvw,4.0f)
    #define SURF_WORLD_TO_CLIPSPACE(position)  GI_WorldToVoxelNDCSpace(position)
    #define SURF_EVALUATE_BxDF EvaluateBxDF_DirectMinimal
    #define SURF_EVALUATE_BxDF_INDIRECT GetIndirectLight_VXGI
    #define SURF_FS_ASSIGN_WORLDPOSITION vs_WORLDPOSITION = GI_FragVoxelToWorldSpace(gl_FragCoord.xyz);
#else
    #if defined(PK_META_PASS_GBUFFER)
        #define SURF_DECLARE_RASTER_OUTPUT out float4 SV_Target0; out float SV_Target1;
        #define SURF_STORE_OUTPUT(value0, value1, worldpos) SV_Target0 = value0; SV_Target1 = value1;
    #else
        #define SURF_DECLARE_RASTER_OUTPUT out float4 SV_Target0;
        #define SURF_STORE_OUTPUT(value0, value1, worldpos) SV_Target0 = value0;
    #endif
    #define SURF_TEX(t, uv) texture(sampler2D(t, pk_Sampler_SurfDefault), uv)
    #define SURF_TEX_TRIPLANAR(t, n, uvw) SampleTexTriplanar(t,n,uvw,0.0f)
    #define SURF_WORLD_TO_CLIPSPACE(position) WorldToClipPos(position)
    #define SURF_EVALUATE_BxDF EvaluateBxDF_Direct
    #define SURF_EVALUATE_BxDF_INDIRECT GetIndirectLight_Main
    #define SURF_FS_ASSIGN_WORLDPOSITION vs_WORLDPOSITION = UVToWorldPos(gl_FragCoord.xy * pk_ScreenParams.zw, ViewDepth(gl_FragCoord.z));
#endif

#if defined(SURF_USE_TANGENTS)
    half3x3 pk_MATRIX_TBN;
    #define SURF_MESH_NORMAL pk_MATRIX_TBN[2]
    float2 SURF_MAKE_PARALLAX_OFFSET(float height, float amount, float3 viewdir) 
    { 
        viewdir = transpose(pk_MATRIX_TBN) * viewdir;
        return (height * amount - amount * 0.5f) * viewdir.xy / (viewdir.z + 0.5f); 
    }
    #define SURF_SAMPLE_NORMAL(normalmap, amount, uv) SampleNormalTex(normalmap, pk_MATRIX_TBN, uv, amount)
    #define SURF_SAMPLE_NORMAL_TRIPLANAR(normalmap, amount, uvw) SampleNormalTexTriplanar(normalmap, pk_MATRIX_TBN, uvw, amount)
    #if defined(PK_USE_DERIVATIVE_TANGENTS)
        #define SURF_VS_ATTRIB_TANGENT
        #define SURF_VS_ASSIGN_TANGENT(index, value)
        #define SURF_FS_ASSIGN_TBN pk_MATRIX_TBN = half3x3(ComposeDerivativeTBN(vs_NORMAL, vs_WORLDPOSITION, vs_TEXCOORD0));
    #else
        #define SURF_VS_ATTRIB_TANGENT PK_DECLARE_VS_ATTRIB(float4 vs_TANGENT);
        #define SURF_VS_ASSIGN_TANGENT(index, value) PK_SET_VS_ATTRIB(vs_TANGENT, index, value);
        #define SURF_FS_ASSIGN_TBN pk_MATRIX_TBN = half3x3(ComposeMikkTBN(vs_NORMAL, vs_TANGENT));
    #endif
#else
    #define SURF_VS_ATTRIB_TANGENT
    #define SURF_VS_ASSIGN_TANGENT(index, value)
    #define SURF_FS_ASSIGN_TBN
    #define SURF_MESH_NORMAL normalize(vs_NORMAL)
    #define SURF_MAKE_PARALLAX_OFFSET(height, amount, viewdir) 0.0f.xx 
    #define SURF_SAMPLE_NORMAL(normalmap, amount, uv) SURF_MESH_NORMAL
    #define SURF_SAMPLE_NORMAL_TRIPLANAR(normalmap, amount, uvw) SURF_MESH_NORMAL
#endif

#if defined(SHADER_STAGE_MESH_TASK)

    void PK_MESHLET_FUNC_TASKLET(inout PKMeshTaskPayload payload)
    {
        Meshlet_Store_FrustumPlanes(pk_WorldToClip);
    }

    bool PK_MESHLET_FUNC_CULL(const PKMeshlet meshlet)
    {
        #if defined(PK_META_PASS_GIVOXELIZE)
        return true;
        #else
        return Meshlet_Cone_Cull(meshlet, pk_ViewWorldOrigin.xyz) && Meshlet_Frustum_Cull(meshlet);
        #endif
    }

//// ---------- VERTEX STAGE ---------- ////
#elif defined(SHADER_STAGE_MESH_ASSEMBLY)

    struct SurfaceVaryings
    {
        float3 worldpos;
        float3 normal;
        float4 tangent;
        float2 texcoord;
    };

    // Use these to modify surface values in vertex stage
    void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf);
    
    #if defined(PK_META_PASS_GIVOXELIZE)
    shared float3 lds_Positions[MAX_VERTICES_PER_MESHLET];
    #endif

    SURF_VS_ATTRIB_TANGENT
    PK_DECLARE_VS_ATTRIB(float3 vs_NORMAL);
    PK_DECLARE_VS_ATTRIB(float2 vs_TEXCOORD0);
    void PK_MESHLET_FUNC_VERTEX(uint vertexIndex, PKVertex vertex, inout float4 sv_Position)
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
        lds_Positions[vertexIndex] = varyings.worldpos;
        #endif

        sv_Position = SURF_WORLD_TO_CLIPSPACE(varyings.worldpos);
        vs_NORMAL[vertexIndex] = varyings.normal;
        vs_TEXCOORD0[vertexIndex] = varyings.texcoord;
        SURF_VS_ASSIGN_TANGENT(vertexIndex, varyings.tangent)
    }

    #if defined(PK_META_PASS_GIVOXELIZE)
    // Cull triangles that should be rasterized through another axis
    void PK_MESHLET_FUNC_TRIANGLE(uint triangleIndex, inout uint3 triangle)
    {
        const float3 a = lds_Positions[triangle.x];
        const float3 b = normalize(lds_Positions[triangle.y] - a);
        const float3 c = normalize(lds_Positions[triangle.z] - a);
        const float3 n = normalize(cross(b, c));

        if (!GI_Test_VX_Normal(n))
        {   
            triangle = uint3(0);
        }
    }
    #endif

//// ---------- FRAGMENT STAGE ---------- ////
#elif defined(SHADER_STAGE_FRAGMENT)
   
    //@TODO move common samplers somewhere else & have some utility logic for using these
    // traditional syntax is a bit cumbersome.
    layout(set = PK_SET_PASS) uniform sampler pk_Sampler_SurfDefault;
    
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
        float depthBias;
    };
    
    #define SurfaceData_Default SurfaceData(0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f.xxx,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f)
    
    // http://www.thetenthplanet.de/archives/1180
    float3x3 ComposeDerivativeTBN(float3 normal, const float3 position, const float2 texcoord)
    {
        normal = normalize(normal);
    
        #if defined(SHADER_STAGE_FRAGMENT)
            const float3 dp1 = dFdxFine(position);
            const float3 dp2 = dFdyFine(position);
            const float2 duv1 = dFdxFine(texcoord);
            const float2 duv2 = dFdyFine(texcoord);
            const float3 dp2perp = cross(dp2, normal);
            const float3 dp1perp = cross(normal, dp1);
            const float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
            const float3 B = dp2perp * duv1.y + dp1perp * duv2.y;
            const float invmax = inversesqrt(max(dot(T,T), dot(B,B)));
            return float3x3(-T * invmax, -B * invmax, normal);
        #else
            return make_TBN(normal);
        #endif
    }
    
    float3x3 ComposeMikkTBN(float3 normal, float4 tangent)
    {
        const float3 T = normalize(tangent.xyz);
        const float3 N = normalize(normal);
        const float3 B = sign(tangent.w) * cross(N, T);
        return float3x3(T, B, N);
    }
    
    float3 SampleNormalTex(const texture2D map, const float3x3 rotation, const float2 uv, const float amount) 
    {
        float3 normal = texture(sampler2D(map, pk_Sampler_SurfDefault), uv).xyz * 2.0f - 1.0f;
        normal = lerp(float3(0,0,1), normal, amount);
        return normalize(rotation * normal); 
    }
    
    float3 SampleNormalTexTriplanar(const texture2D tex, const float3x3 rotation, const float3 uvw, const float amount) 
    {
        float3 blend = abs(rotation[2]);
        blend /= dot(blend, 1.0.xxx);
        const float3 cx = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.yz).xyz;
        const float3 cy = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.xz).xyz;
        const float3 cz = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.xy).xyz;
        float3 normal = (cx * blend.x + cy * blend.y + cz * blend.z) * 2.0f - 1.0f;
        normal = lerp(float3(0,0,1), normal, amount);
        return normalize(rotation * normal); 
    }
    
    float4 SampleTexTriplanar(const texture2D tex, const float3 normal, const float3 uvw, float bias)
    {
        float3 blend = abs(normal);
        blend /= dot(blend, 1.0.xxx);
        #if defined(SHADER_STAGE_FRAGMENT)
        const float4 cx = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.yz, bias);
        const float4 cy = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.xz, bias);
        const float4 cz = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.xy, bias);
        #else
        const float4 cx = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.yz);
        const float4 cy = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.xz);
        const float4 cz = texture(sampler2D(tex, pk_Sampler_SurfDefault), uvw.xy);
        #endif
        return cx * blend.x + cy * blend.y + cz * blend.z;
    }
    
    float3 GetIndirectLight_Main(const BxDFSurf surf, const float3 worldpos, const float3 clipuvw)
    {
        //float3 diffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
        //float3 specular = SampleEnvironment(OctaUV(reflect(-surf.viewdir, surf.normal)), surf.alpha);
    
        float3 diffuse = GI_Load_Resolved_Diff(clipuvw.xy);
        float3 specular = GI_Load_Resolved_Spec(clipuvw.xy);
    
        float3 color = 0.0f.xxx;
        color += EvaluateBxDF_Indirect(surf, diffuse, specular);
    
        // Optional approximate specular details from diffuse sh
        color += GI_ShadeRoughSpecularDetails(surf, clipuvw.xy);
    
        return color;
    }
    
    // Multi bounce gi. Causes some very lingering light artifacts & bleeding. @TODO Consider adding a setting for this.
    float3 GetIndirectLight_VXGI(const BxDFSurf surf, const float3 worldpos, const float3 clipuvw)
    {
        // Get unquantized clip uvw.
        const float deltaDepth = SampleViewDepth(clipuvw.xy) - ViewDepth(clipuvw.z); 

        // Fragment is in view
        [[branch]]
        if (deltaDepth > -0.01f && deltaDepth < 0.1f)
        {
            // Sample screen space SH values for more accurate results.
            return surf.albedo * GI_Load_Resolved_Diff(clipuvw.xy);
        }
        else
        {
            const float3 environmentDiffuse = SampleEnvironment(OctaUV(surf.normal), 1.0f);
            const float4 tracedDiffuse = GI_ConeTrace_Diffuse(worldpos, surf.normal);
            return surf.albedo * (environmentDiffuse * tracedDiffuse.a + tracedDiffuse.rgb);
        }
    }

    // Use these to modify surface values in fragment stage
    void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf);

    float3 vs_WORLDPOSITION;
    SURF_VS_ATTRIB_TANGENT
    PK_DECLARE_VS_ATTRIB(float3 vs_NORMAL);
    PK_DECLARE_VS_ATTRIB(float2 vs_TEXCOORD0);
    SURF_DECLARE_RASTER_OUTPUT
    layout(early_fragment_tests) in;
    void main()
    {
        SURF_FS_ASSIGN_WORLDPOSITION
        SURF_FS_ASSIGN_TBN

        SurfaceData surf = SurfaceData_Default;
        surf.viewdir = normalize(pk_ViewWorldOrigin.xyz - vs_WORLDPOSITION);
        surf.worldpos = vs_WORLDPOSITION;
        surf.normal = float3(0,0,1);
        surf.albedo = 1.0f.xxx;
        surf.alpha = 1.0f;
        surf.roughness = 1.0f;
        
        #if defined(PK_META_PASS_GIVOXELIZE)
            float3 voxelPos = GI_QuantizeWorldToVoxelSpace(surf.worldpos);
            voxelPos = WorldToClipUVW(voxelPos);

            [[branch]]
            if (!Test_InUVW(voxelPos) || GI_Test_VX_HasValue(surf.worldpos))                 
            {
                return;
            }

            surf.clipuvw = WorldToClipUVW(surf.worldpos);
            surf.clipuvw.xy = ClampUVScreenBorder(surf.clipuvw.xy);
            int2 screencoord = int2(surf.clipuvw.xy * pk_ScreenSize.xy);
        #else
            surf.clipuvw = float3(gl_FragCoord.xy * pk_ScreenParams.zw, gl_FragCoord.z);
            int2 screencoord = int2(gl_FragCoord.xy);
        #endif

        SURF_FUNCTION_FRAGMENT(vs_TEXCOORD0, surf);

        #if !defined(PK_META_PASS_GIVOXELIZE)
            const float shiftAmount = dot(surf.normal, surf.viewdir);
            surf.normal = shiftAmount < 0.0f ? surf.normal + surf.viewdir * (-shiftAmount + 1e-5f) : surf.normal;
            surf.roughness = max(surf.roughness, 0.002f);
        #endif

        float4 sv_output0 = 0.0f.xxxx;
        float sv_output1 = 0.0f;

        #if defined(PK_META_PASS_GBUFFER)
            sv_output0 = EncodeGBufferWorldNR(surf.normal, surf.roughness, surf.metallic);
            sv_output1 = EncodeBiasedDepth(surf.clipuvw.z, dot(surf.viewdir, surf.normal), surf.depthBias);
        #else
            const float3 F0 = lerp(PK_DIELECTRIC_SPEC.rgb, surf.albedo, surf.metallic);
            const float reflectivity = PK_DIELECTRIC_SPEC.r + surf.metallic * PK_DIELECTRIC_SPEC.a;
            surf.albedo *= 1.0f - reflectivity;

            #if defined(SURF_TRANSPARENT)
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

            sv_output0.rgb = SURF_EVALUATE_BxDF_INDIRECT(bxdf_surf, surf.worldpos, surf.clipuvw);

            #if !defined(PK_META_PASS_GIVOXELIZE)
                sv_output0.rgb *= surf.occlusion;
            #endif

            LightTile tile = Lights_GetTile_PX(screencoord, ViewDepth(surf.clipuvw.z));
            for (uint i = tile.start; i < tile.end; ++i)
            {
                Light light = GetLight(i, surf.worldpos, surf.normal, tile.cascade);
                sv_output0.rgb += SURF_EVALUATE_BxDF(bxdf_surf, light.direction, light.color, light.shadow, light.sourceRadius);
            }

            sv_output0.rgb += surf.emission;
            sv_output0.a = surf.alpha;
        #endif

        SURF_STORE_OUTPUT(sv_output0, sv_output1, surf.worldpos)
    }

#endif