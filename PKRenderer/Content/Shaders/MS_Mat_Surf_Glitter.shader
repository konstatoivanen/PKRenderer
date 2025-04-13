
#pragma pk_material_property float4 _Color
#pragma pk_material_property float _Metallic
#pragma pk_material_property float _Roughness
#pragma pk_material_property float _Occlusion
#pragma pk_material_property float _NormalAmount
#pragma pk_material_property float _HeightAmount
#pragma pk_material_property texture2D _AlbedoTexture
#pragma pk_material_property texture2D _PBSTexture
#pragma pk_material_property texture2D _NormalMap
#pragma pk_material_property texture2D _HeightMap

#define BxDF_ENABLE_SHEEN
#define SURF_USE_TANGENTS

#include "includes/SurfaceShaderBase.glsl"
#include "includes/NoiseBlue.glsl"

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float height = SURF_TEX(_HeightMap, uv).x;
    uv += SURF_MAKE_PARALLAX_OFFSET(height, _HeightAmount, surf.view_dir);

    float3 pbs_texture_val = SURF_TEX(_PBSTexture, uv).xyz;
    surf.metallic = pbs_texture_val.SRC_METALLIC * _Metallic;
    // @TODO this is a hack to fix a bad sand texture. Replace sand mat with a better one & get rid of this.
    surf.roughness = sqrt(pbs_texture_val.SRC_ROUGHNESS * _Roughness);
    surf.occlusion = lerp(1.0f, pbs_texture_val.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = SURF_TEX(_AlbedoTexture, uv).rgb * _Color.rgb;
    surf.depth_bias = SURF_TEX(_HeightMap, uv).x * _HeightAmount;
    surf.depth_bias = pow3(surf.depth_bias);

    // Add glitter
    float t = GlobalNoiseBlueUv(uv).r;
    t = pow5(t);
    t -= 0.875f;
    t *= 50.0f;
    t = max(0.0f, t);
    surf.roughness -= t;

    surf.sheen_roughness = 0.9f;
    surf.sheen = surf.albedo * 0.2f;
};
