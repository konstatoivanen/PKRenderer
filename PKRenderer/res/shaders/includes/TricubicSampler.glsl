#pragma once

// Source: https://www.shadertoy.com/view/cts3Rj

#define DEFINE_TRICUBIC_SAMPLER(TVolume, TVolumeSize)                           \
                                                                                \
    float4 TVolume##SAMPLER_TRICUBIC(float3 coord)                              \
    {                                                                           \
        float3 texSize = TVolumeSize;                                           \
        float3 coord_grid = coord * texSize - 0.5;                              \
        float3 index = floor(coord_grid);                                       \
        float3 fraction = coord_grid - index;                                   \
        float3 one_frac = 1.0 - fraction;                                       \
        float3 w0 = 1.0 / 6.0 * one_frac * one_frac * one_frac;                 \
        float3 w1 = 2.0 / 3.0 - 0.5 * fraction * fraction * (2.0 - fraction);   \
        float3 w2 = 2.0 / 3.0 - 0.5 * one_frac * one_frac * (2.0 - one_frac);   \
        float3 w3 = 1.0 / 6.0 * fraction * fraction * fraction;                 \
        float3 g0 = w0 + w1;                                                    \
        float3 g1 = w2 + w3;                                                    \
        float3 mult = 1.0 / texSize;                                            \
        float3 h0 = mult * ((w1 / g0) - 0.5 + index);                           \
        float3 h1 = mult * ((w3 / g1) + 1.5 + index);                           \
        float4 tex000 = textureLod(TVolume, h0, 0.);                            \
        float4 tex100 = textureLod(TVolume, float3(h1.x, h0.y, h0.z), 0.0);     \
        tex000 = mix(tex100, tex000, g0.x);                                     \
        float4 tex010 = textureLod(TVolume, float3(h0.x, h1.y, h0.z), 0.0);     \
        float4 tex110 = textureLod(TVolume, float3(h1.x, h1.y, h0.z), 0.0);     \
        tex010 = mix(tex110, tex010, g0.x);                                     \
        tex000 = mix(tex010, tex000, g0.y);                                     \
        float4 tex001 = textureLod(TVolume, float3(h0.x, h0.y, h1.z), 0.0);     \
        float4 tex101 = textureLod(TVolume, float3(h1.x, h0.y, h1.z), 0.0);     \
        tex001 = mix(tex101, tex001, g0.x);                                     \
        float4 tex011 = textureLod(TVolume, float3(h0.x, h1.y, h1.z), 0.0);     \
        float4 tex111 = textureLod(TVolume, h1, 0.0);                           \
        tex011 = mix(tex111, tex011, g0.x);                                     \
        tex001 = mix(tex011, tex001, g0.y);                                     \
        return mix(tex001, tex000, g0.z);                                       \
    }                                                                           \
                                                                                \

#define SAMPLE_TRICUBIC(TVolume, coord) TVolume##SAMPLER_TRICUBIC(coord)