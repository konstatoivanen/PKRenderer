
#pragma pk_material_property float4 _Color
#pragma pk_material_property float _Metallic
#pragma pk_material_property float _Roughness
#pragma pk_material_property float _Occlusion
#pragma pk_material_property float _NormalAmount
#pragma pk_material_property texture2D _AlbedoTexture
#pragma pk_material_property texture2D _PBSTexture
#pragma pk_material_property texture2D _NormalMap

#define BxDF_ENABLE_SHEEN
#define BxDF_ENABLE_CLEARCOAT
#define SURF_USE_TANGENTS
#include "includes/SurfaceShaderBase.glsl"

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float3 pbs_texture_val = SURF_TEX_TRIPLANAR(_PBSTexture, SURF_MESH_NORMAL, surf.world_pos * 0.25f).xyz;
    surf.metallic = pbs_texture_val.SRC_METALLIC * _Metallic;
    surf.roughness = pbs_texture_val.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, pbs_texture_val.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL_TRIPLANAR(_NormalMap, _NormalAmount, surf.world_pos * 0.25f);
    surf.albedo = SURF_TEX_TRIPLANAR(_AlbedoTexture, SURF_MESH_NORMAL, surf.world_pos * 0.25f).rgb * _Color.xyz;
    surf.sheen = float3(0.68f, 0.56f, 0.51f) * 0.5f;
    surf.sheen_roughness = 0.9f;
    surf.clear_coat = 0.2f;
    surf.clear_coat_gloss = 0.0f;
}
