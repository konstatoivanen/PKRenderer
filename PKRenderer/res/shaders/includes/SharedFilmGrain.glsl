#pragma once
#include SharedPostEffects.glsl
#include Constants.glsl

PK_DECLARE_SET_DRAW uniform sampler2D pk_FilmGrainTex;

float3 FilmGrain(float3 color, float2 coord)
{
    coord /= textureSize(pk_FilmGrainTex, 0).xy * 2.0f;
    float3 grain = tex2D(pk_FilmGrainTex, coord).rgb;
    float lum = 1.0 - sqrt(dot(pk_Luminance.xyz, saturate(color)));
    lum = lerp(1.0, lum, pk_VignetteGrain.z);
    return color.rgb + color.rgb * grain * pk_VignetteGrain.w * lum;
}