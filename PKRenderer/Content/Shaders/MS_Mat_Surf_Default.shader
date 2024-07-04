#PK_MaterialProperty float4 _Color
#PK_MaterialProperty float4 _EmissionColor
#PK_MaterialProperty float _Metallic
#PK_MaterialProperty float _Roughness
#PK_MaterialProperty float _Occlusion
#PK_MaterialProperty float _NormalAmount
#PK_MaterialProperty float _HeightAmount
#PK_MaterialProperty texture2D _AlbedoTexture
#PK_MaterialProperty texture2D _PBSTexture
#PK_MaterialProperty texture2D _NormalMap
#PK_MaterialProperty texture2D _HeightMap
#PK_MaterialProperty texture2D _EmissionTexture

#define SURF_USE_TANGENTS

#include "includes/SurfaceShaderBase.glsl"

#pragma PROGRAM_MESH_TASK
// Surface shader handles this

#pragma PROGRAM_MESH_ASSEMBLY

void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf) {}

#pragma PROGRAM_FRAGMENT
void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float height = SURF_TEX(_HeightMap, uv).x;
    uv += SURF_MAKE_PARALLAX_OFFSET(height, _HeightAmount, surf.viewdir);

    //// GI color test code
    //float lval = surf.worldpos.x * 0.025f + pk_Time.y * 0.25f;
    //lval -= floor(lval);
    //lval = 1.0f - lval;
    //lval -= 0.75f;
    //lval *= 4.0f;
    //lval = saturate(lval);
    //lval *= pow5(lval);
    //float3 c = HSVToRGB(0.025f, 0.8f, lval * 20.0f);
    //
    //surf.emission = texture(sampler2D(_EmissionTexture, pk_Sampler_SurfDefault), uv * 4.0f).xxx * c * _EmissionColor.rgb;//PK_ACCESS_INSTANCED_PROP(_EmissionColor).rgb;

    /*
        float lval = surf.worldpos.y * 2.0f - 0.01f;
        lval -= floor(lval);
        lval = 1.0f - lval;
        lval -= 0.7f;
        lval *= 100.0f;
        lval = saturate(lval);
        lval *= pow5(lval);

        lval *= step(0.1f, surf.worldpos.z + 0.01f - floor(surf.worldpos.z + 0.01f));

        int2 offs = int2(GlobalNoiseBlue(int2(surf.worldpos.yx * 2 + 0.5f)).yz * 256.0f);

        float3 noise0 = GlobalNoiseBlue(int2(surf.worldpos.xz * 0.5f + 0.75f) + int2(pk_Time.xy * 0.1f) + offs);

        lval *= step(0.6f, noise0.y);

        float3 e = int3(GlobalNoiseBlue(int2(surf.worldpos.xz * 0.1f)) * 256.0f) / 256.0f;

        float3 c = float3(e.x * 0.1f + 0.5f, 1.0f - e.z * 0.5f, lval * noise0.x * 12.0f);

        float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
        float3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
        float3 ecolor = c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);

        surf.emission = ecolor;//PK_ACCESS_INSTANCED_PROP(_EmissionColor).rgb;
    */

    float3 textureval = SURF_TEX(_PBSTexture, uv).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = SURF_TEX(_AlbedoTexture, uv).rgb * _Color.xyz;
    surf.depthBias = SURF_TEX(_HeightMap, uv).x * _HeightAmount;
}