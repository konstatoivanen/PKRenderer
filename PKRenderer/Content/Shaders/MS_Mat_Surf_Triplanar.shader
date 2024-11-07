
#pragma pk_material_property float4 _Color
#pragma pk_material_property float _Metallic
#pragma pk_material_property float _Roughness
#pragma pk_material_property float _Occlusion
#pragma pk_material_property float _NormalAmount
#pragma pk_material_property texture2D _AlbedoTexture
#pragma pk_material_property texture2D _PBSTexture
#pragma pk_material_property texture2D _NormalMap

//#define BxDF_ENABLE_SHEEN
//#define BxDF_ENABLE_CLEARCOAT
#define SURF_USE_TANGENTS
#include "includes/SurfaceShaderBase.glsl"

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float3 textureval = SURF_TEX_TRIPLANAR(_PBSTexture, SURF_MESH_NORMAL, surf.worldpos * 0.25f).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL_TRIPLANAR(_NormalMap, _NormalAmount, surf.worldpos * 0.25f);
    surf.albedo = SURF_TEX_TRIPLANAR(_AlbedoTexture, SURF_MESH_NORMAL, surf.worldpos * 0.25f).rgb * _Color.xyz;
    surf.sheen = float3(0.68f, 0.56f, 0.51f) * 0.2f;
    surf.sheenRoughness = 0.9f;
    surf.clearCoat = 0.5f;
    surf.clearCoatGloss = 0.0f;
}
