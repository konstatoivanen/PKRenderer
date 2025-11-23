
#pragma pk_material_property float4 _Color
#pragma pk_material_property float4 _EmissionColor
#pragma pk_material_property float _Metallic
#pragma pk_material_property float _Roughness
#pragma pk_material_property float _Occlusion
#pragma pk_material_property float _NormalAmount
#pragma pk_material_property float _HeightAmount
#pragma pk_material_property texture2D _AlbedoTexture
#pragma pk_material_property texture2D _PBSTexture
#pragma pk_material_property texture2D _NormalMap
#pragma pk_material_property texture2D _HeightMap
#pragma pk_material_property texture2D _EmissionTexture

#define SURF_USE_TANGENTS
#include "includes/SurfaceShaderBase.glsl"

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float height = SURF_TEX(_HeightMap, uv).x;
    uv += SURF_MAKE_PARALLAX_OFFSET(height, _HeightAmount, surf.view_dir);

    //// GI color test code
//#if !defined(SHADER_STAGE_MESH_TASK)
//    float lval = surf.world_pos.x * 0.025f + pk_Time.y * 0.25f;
//    lval -= floor(lval);
//    lval = 1.0f - lval;
//    lval -= 0.75f;
//    lval *= 4.0f;
//    lval = saturate(lval);
//    lval *= pow5(lval);
//    float3 c = HsvToRgb(0.025f, 0.8f, lval * 20.0f);
//    
//    surf.emission = texture(sampler2D(_EmissionTexture, pk_Sampler_SurfDefault), uv * 4.0f).xxx * c * _EmissionColor.rgb;//PK_ACCESS_INSTANCED_PROP(_EmissionColor).rgb;
//#endif

    /*
        float lval = surf.world_pos.y * 2.0f - 0.01f;
        lval -= floor(lval);
        lval = 1.0f - lval;
        lval -= 0.7f;
        lval *= 100.0f;
        lval = saturate(lval);
        lval *= pow5(lval);

        lval *= step(0.1f, surf.world_pos.z + 0.01f - floor(surf.world_pos.z + 0.01f));

        int2 offs = int2(GlobalNoiseBlue(int2(surf.world_pos.yx * 2 + 0.5f)).yz * 256.0f);

        float3 noise0 = GlobalNoiseBlue(int2(surf.world_pos.xz * 0.5f + 0.75f) + int2(pk_Time.xy * 0.1f) + offs);

        lval *= step(0.6f, noise0.y);

        float3 e = int3(GlobalNoiseBlue(int2(surf.world_pos.xz * 0.1f)) * 256.0f) / 256.0f;

        float3 c = float3(e.x * 0.1f + 0.5f, 1.0f - e.z * 0.5f, lval * noise0.x * 12.0f);

        float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        float3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        float3 ecolor = c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);

        surf.emission = ecolor;//PK_ACCESS_INSTANCED_PROP(_EmissionColor).rgb;
    */

    float3 pbs_texture_val = SURF_TEX(_PBSTexture, uv).xyz;
    surf.metallic = pbs_texture_val.SRC_METALLIC * _Metallic;
    surf.roughness = pbs_texture_val.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, pbs_texture_val.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = SURF_TEX(_AlbedoTexture, uv).rgb * _Color.xyz;
    surf.depth_bias = SURF_TEX(_HeightMap, uv).x * _HeightAmount;
}
