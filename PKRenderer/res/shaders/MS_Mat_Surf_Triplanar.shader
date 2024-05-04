#MaterialProperty float4 _Color
#MaterialProperty float _Metallic
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap

#define BxDF_ENABLE_SHEEN
#define BxDF_ENABLE_CLEARCOAT
#define SURF_USE_TANGENTS
#include includes/SurfaceShaderBase.glsl

#pragma PROGRAM_MESH_TASK
// Surface shader handles this

#pragma PROGRAM_MESH_ASSEMBLY

void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf) {}

#pragma PROGRAM_FRAGMENT

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float3 textureval = SURF_TEX_TRIPLANAR(_PBSTexture, SURF_MESH_NORMAL, surf.worldpos * 0.25f).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL_TRIPLANAR(_NormalMap, _NormalAmount, surf.worldpos * 0.25f);
    surf.albedo = SURF_TEX_TRIPLANAR(_AlbedoTexture, SURF_MESH_NORMAL, surf.worldpos * 0.25f).rgb * _Color.xyz;
    surf.sheen = 1.0f.xxx;
    surf.sheenTint = 0.0f;
    surf.clearCoat = 0.5f.xxx;
    surf.clearCoatGloss = 0.0f;
    //    surf.depthBias = dot(float3(SURF_MESH_NORMAL), surf.normal) * 0.04f;
}