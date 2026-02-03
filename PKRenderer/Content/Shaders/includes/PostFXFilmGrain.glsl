#pragma once
#include "PostFXResources.glsl"
#include "Constants.glsl"

uniform sampler2D pk_FilmGrain_Texture;

float3 FilmGrain(const float2 coord, const float3 color, const float exposure)
{
    const float2 uv = coord / textureSize(pk_FilmGrain_Texture, 0).xy;
    
    float3 grain = texture(pk_FilmGrain_Texture, uv).rgb;
    grain *= lerp(1.0f, exposure, pk_FilmGrain_ExposureSensitivity);
    grain *= exp(-sqrt(color) * pk_FilmGrain_Luminance);
    grain *= pk_FilmGrain_Intensity;

    return color * max(0.0f.xxx, 1.0f.xxx - grain);
}
