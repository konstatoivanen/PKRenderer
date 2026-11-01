#pragma once
#include "PostFXResources.glsl"
#include "Constants.glsl"

uniform Texture2D<float3> pk_FilmGrain_Texture;

float3 FilmGrain(const float2 coord, const float3 color, const float exposure)
{
    const float2 uv = coord / pk_FilmGrain_Texture.GetSize();

    float3 grain = pk_FilmGrain_Texture.SampleLevel(pk_SamplerBilinearRepeat, uv, 0.0f).rgb;
    grain *= lerp(1.0f, exposure, pk_View.FilmGrain_ExposureSensitivity);
    grain *= exp(-sqrt(color) * pk_View.FilmGrain_Luminance);
    grain *= pk_View.FilmGrain_Intensity;

    return color * max(0.0f.xxx, 1.0f.xxx - grain);
}
