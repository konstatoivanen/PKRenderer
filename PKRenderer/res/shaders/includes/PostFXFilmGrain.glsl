#pragma once
#include PostFXResources.glsl
#include Constants.glsl

PK_DECLARE_SET_DRAW uniform sampler2D pk_FilmGrain_Texture;

float3 FilmGrain(float3 color, float2 coord)
{
    coord /= textureSize(pk_FilmGrain_Texture, 0).xy * 2.0f;
    float3 grain = texture(pk_FilmGrain_Texture, coord).rgb;
    float lum = 1.0 - sqrt(dot(PK_LUMA_BT709, saturate(color)));
    lum = lerp(1.0, lum, pk_FilmGrain_Luminance);
    return color.rgb + color.rgb * grain * pk_FilmGrain_Intensity * lum;
}