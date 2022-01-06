#version 460
#ZTest LEqual
#ZWrite True
#Cull Back
#MaterialProperty float4 _Color
#MaterialProperty float4 _EmissionColor
#MaterialProperty float _Metallic
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty float _HeightAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap
#MaterialProperty texture2D _HeightMap
#MaterialProperty texture2D _EmissionTexture
#define PK_NORMALMAPS
#define PK_HEIGHTMAPS
#include includes/SharedSurfaceShading.glsl

#pragma PROGRAM_VERTEX
void PK_SURFACE_FUNC_VERT(inout SurfaceFragmentVaryings surf) {}

#pragma PROGRAM_FRAGMENT
void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf)
{
    float2 uv = varyings.vs_TEXCOORD0 + PK_SURF_SAMPLE_PARALLAX_OFFSET(_HeightMap, _HeightAmount);

    //// GI color test code
    //float lval = surf.worldpos.z * 0.025f + pk_Time.y * 0.25f;
    //lval -= floor(lval);
    //lval = 1.0f - lval;
    //lval -= 0.75f;
    //lval *= 4.0f;
    //lval = saturate(lval);
    //lval *= pow5(lval);
    //float3 c = float3(surf.worldpos.z * 0.025f + pk_Time.y, 1.0f, lval * 20.0f);
    //
    //float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    //float3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    //float3 ecolor = c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
    // 
    //surf.emission = tex2D(PK_ACCESS_INSTANCED_PROP(_EmissionTexture), uv * 8.0f).rgb * ecolor;//PK_ACCESS_INSTANCED_PROP(_EmissionColor).rgb;

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

    float3 textureval = tex2D(_PBSTexture, uv).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = tex2D(_AlbedoTexture, uv).rgb * _Color.xyz;
}