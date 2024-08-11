#pragma once
#include "PostFXResources.glsl"
#include "Constants.glsl"

PK_DECLARE_SET_DRAW uniform sampler2D pk_FilmGrain_Texture;

float3 FilmGrain(const float2 coord, const float3 color, const float exposure)
{
    const float2 uv = coord / textureSize(pk_FilmGrain_Texture, 0).xy;
    const float3 grain = texture(pk_FilmGrain_Texture, uv).rgb * 2.0f - 1.0f;
    const float exposureReduction = lerp(1.0f / exposure, 1.0f, pk_FilmGrain_ExposureSensitivity);
    const float lumaReduction = exp(-sqrt(dot(PK_LUMA_BT709, color)) * pk_FilmGrain_Luminance);
    return max(0.0f.xxx, color + color * grain * pk_FilmGrain_Intensity * lumaReduction * exposureReduction);
}