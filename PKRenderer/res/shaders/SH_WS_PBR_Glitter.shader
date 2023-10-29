#version 460
#MaterialProperty float4 _Color
#MaterialProperty float _Metallic
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty float _HeightAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap
#MaterialProperty texture2D _HeightMap

#define BxDF_ENABLE_SHEEN
#define SURF_USE_TANGENTS
#include includes/SurfaceShaderBase.glsl

#pragma PROGRAM_VERTEX
void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf) {}

#pragma PROGRAM_FRAGMENT
void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float height = SURF_TEX(_HeightMap, uv).x;
    uv += SURF_MAKE_PARALLAX_OFFSET(height, _HeightAmount, surf.viewdir);

    float3 textureval = SURF_TEX(_PBSTexture, uv).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    // @TODO this is a hack to fix a bad sand texture. Replace sand mat with a better one & get rid of this.
    surf.roughness = sqrt(textureval.SRC_ROUGHNESS * _Roughness); 
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = SURF_TEX(_AlbedoTexture, uv).rgb * _Color.rgb;
    surf.depthBias = SURF_TEX(_HeightMap, uv).x * _HeightAmount * 10.0f;
    surf.depthBias = pow3(surf.depthBias);

    // Add glitter
    float t = GlobalNoiseBlueUV(uv).r;
    t = pow5(t);
    t -= 0.875f;
    t *= 50.0f;
    t = max(0.0f, t);
    surf.roughness -= t;

    surf.sheenTint = 0.5f;
    surf.sheen = 1.0f.xxx;
};