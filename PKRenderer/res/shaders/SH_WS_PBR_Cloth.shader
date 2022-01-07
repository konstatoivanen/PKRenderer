#version 460
#MaterialProperty float4 _Color
#MaterialProperty float4 _SheenColor
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty float _HeightAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap
#MaterialProperty texture2D _HeightMap
#define PK_NORMALMAPS
#define PK_ACTIVE_BRDF BRDF_PBS_CLOTH_DIRECT
#define PK_ACTIVE_VXGI_BRDF BRDF_VXGI_CLOTH
#include includes/SharedSurfaceShading.glsl

#pragma PROGRAM_VERTEX
void PK_SURFACE_FUNC_VERT(inout SurfaceFragmentVaryings surf) {}

#pragma PROGRAM_FRAGMENT
void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf)
{
    float2 uv = varyings.vs_TEXCOORD0 + PK_SURF_SAMPLE_PARALLAX_OFFSET(_HeightMap, _HeightAmount);
    float3 textureval = tex2D(_PBSTexture, uv).xyz;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.subsurface_distortion = 0.1f;
    surf.subsurface_power = 2.0f;
    surf.subsurface_thickness = 0.4f;
    surf.sheen = _SheenColor.rgb;
    surf.normal = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = tex2D(_AlbedoTexture, uv).rgb * _Color.rgb;
};