#PK_MaterialProperty float4 _Color
#PK_MaterialProperty float4 _SheenColor
#PK_MaterialProperty float _Roughness
#PK_MaterialProperty float _Occlusion
#PK_MaterialProperty float _NormalAmount
#PK_MaterialProperty float _HeightAmount
#PK_MaterialProperty texture2D _AlbedoTexture
#PK_MaterialProperty texture2D _PBSTexture
#PK_MaterialProperty texture2D _NormalMap
#PK_MaterialProperty texture2D _HeightMap

#define BxDF_ENABLE_SUBSURFACE
#define BxDF_ENABLE_SHEEN
#define SURF_USE_TANGENTS
#include "includes/SurfaceShaderBase.glsl"

#pragma PROGRAM_MESH_TASK
// Surface shader handles this

#pragma PROGRAM_MESH_ASSEMBLY

void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf) {}

#pragma PROGRAM_FRAGMENT
void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    //uv += PK_SURF_SAMPLE_PARALLAX_OFFSET(_HeightMap, _HeightAmount, uv, surf.viewdir);

    float3 textureval = SURF_TEX(_PBSTexture, uv).xyz;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.subsurface = 0.1f.xxx;
    surf.sheen = _SheenColor.rgb;
    surf.normal = SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = SURF_TEX(_AlbedoTexture, uv).rgb * _Color.rgb;
};