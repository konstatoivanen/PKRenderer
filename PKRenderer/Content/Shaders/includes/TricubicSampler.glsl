#pragma once

// Source: https://www.shadertoy.com/view/cts3Rj

#define DEFINE_TRICUBIC_SAMPLER(TVolume, TVolumeSize)                                   \
                                                                                        \
    half4 TVolume##SAMPLER_TRICUBIC(float3 uvw)                                         \
    {                                                                                   \
        half3 tex_size = half3(TVolumeSize);                                            \
        half3 coord = half3(uvw) * tex_size - 0.5hf;                                    \
        half3 index = floor(coord);                                                     \
        half3 fraction = coord - index;                                                 \
        half3 one_frac = 1.0hf - fraction;                                              \
        half3 w0 = 1.0hf / 6.0hf * one_frac * one_frac * one_frac;                      \
        half3 w1 = 2.0hf / 3.0hf - 0.5hf * fraction * fraction * (2.0hf - fraction);    \
        half3 w2 = 2.0hf / 3.0hf - 0.5hf * one_frac * one_frac * (2.0hf - one_frac);    \
        half3 w3 = 1.0hf / 6.0hf * fraction * fraction * fraction;                      \
        half3 g0 = w0 + w1;                                                             \
        half3 g1 = w2 + w3;                                                             \
        half3 mult = 1.0hf / tex_size;                                                  \
        half3 h0 = mult * ((w1 / g0) - 0.5hf + index);                                  \
        half3 h1 = mult * ((w3 / g1) + 1.5hf + index);                                  \
        half4 tex000 = half4(textureLod(TVolume, h0, 0.0));                             \
        half4 tex100 = half4(textureLod(TVolume, float3(h1.x, h0.y, h0.z), 0.0));       \
        half4 tex010 = half4(textureLod(TVolume, float3(h0.x, h1.y, h0.z), 0.0));       \
        half4 tex110 = half4(textureLod(TVolume, float3(h1.x, h1.y, h0.z), 0.0));       \
        half4 tex001 = half4(textureLod(TVolume, float3(h0.x, h0.y, h1.z), 0.0));       \
        half4 tex101 = half4(textureLod(TVolume, float3(h1.x, h0.y, h1.z), 0.0));       \
        half4 tex011 = half4(textureLod(TVolume, float3(h0.x, h1.y, h1.z), 0.0));       \
        half4 tex111 = half4(textureLod(TVolume, h1, 0.0));                             \
        tex000 = mix(tex100, tex000, g0.x);                                             \
        tex010 = mix(tex110, tex010, g0.x);                                             \
        tex000 = mix(tex010, tex000, g0.y);                                             \
        tex001 = mix(tex101, tex001, g0.x);                                             \
        tex011 = mix(tex111, tex011, g0.x);                                             \
        tex001 = mix(tex011, tex001, g0.y);                                             \
        return mix(tex001, tex000, g0.z);                                               \
    }                                                                                   \
                                                                                        \

#define SAMPLE_TRICUBIC(TVolume, coord) TVolume##SAMPLER_TRICUBIC(coord)
