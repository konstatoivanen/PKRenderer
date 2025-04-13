
#pragma pk_material_property float4 _Color
#pragma pk_material_property float4 _SheenColor
#pragma pk_material_property float _Roughness
#pragma pk_material_property float _Occlusion
#pragma pk_material_property float _NormalAmount
#pragma pk_material_property float _HeightAmount
#pragma pk_material_property texture2D _AlbedoTexture
#pragma pk_material_property texture2D _PBSTexture
#pragma pk_material_property texture2D _NormalMap
#pragma pk_material_property texture2D _HeightMap

#define BxDF_ENABLE_SUBSURFACE
#define BxDF_ENABLE_SHEEN
#define SURF_USE_TANGENTS
#include "includes/SurfaceShaderBase.glsl"

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    //uv += PK_SURF_SAMPLE_PARALLAX_OFFSET(_HeightMap, _HeightAmount, uv, surf.view_dir);
    float3 pbs_texture_val = SURF_TEX(_PBSTexture, uv).xyz;
    surf.roughness = pbs_texture_val.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, pbs_texture_val.SRC_OCCLUSION, _Occlusion);
    surf.subsurface = 0.1f.xxx;
    surf.sheen = _SheenColor.rgb;
    surf.normal = SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = SURF_TEX(_AlbedoTexture, uv).rgb * _Color.rgb;
};
