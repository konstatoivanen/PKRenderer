#pragma once

// Source: https://www.shadertoy.com/view/cts3Rj

#define DEFINE_TRICUBIC_SAMPLER(TVolume, TVolumeSize)                                                          \
                                                                                                               \
    half4 TVolume##SAMPLER_TRICUBIC(float3 uvw)                                                                \
    {                                                                                                          \
        half3 tex_size = half3(TVolumeSize);                                                                   \
        half3 coord = half3(uvw) * tex_size - 0.5h;                                                            \
        half3 index = floor(coord);                                                                            \
        half3 fraction = coord - index;                                                                        \
        half3 one_frac = 1.0h - fraction;                                                                      \
        half3 w0 = 1.0h / 6.0h * one_frac * one_frac * one_frac;                                               \
        half3 w1 = 2.0h / 3.0h - 0.5h * fraction * fraction * (2.0h - fraction);                               \
        half3 w2 = 2.0h / 3.0h - 0.5h * one_frac * one_frac * (2.0h - one_frac);                               \
        half3 w3 = 1.0h / 6.0h * fraction * fraction * fraction;                                               \
        half3 g0 = w0 + w1;                                                                                    \
        half3 g1 = w2 + w3;                                                                                    \
        half3 mult = 1.0h / tex_size;                                                                          \
        half3 h0 = mult * ((w1 / g0) - 0.5h + index);                                                          \
        half3 h1 = mult * ((w3 / g1) + 1.5h + index);                                                          \
        half4 tex000 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, h0, 0.0));                        \
        half4 tex100 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, float3(h1.x, h0.y, h0.z), 0.0));  \
        half4 tex010 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, float3(h0.x, h1.y, h0.z), 0.0));  \
        half4 tex110 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, float3(h1.x, h1.y, h0.z), 0.0));  \
        half4 tex001 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, float3(h0.x, h0.y, h1.z), 0.0));  \
        half4 tex101 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, float3(h1.x, h0.y, h1.z), 0.0));  \
        half4 tex011 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, float3(h0.x, h1.y, h1.z), 0.0));  \
        half4 tex111 = half4(TVolume.SampleLevel(pk_SamplerTrilinearClamped, h1, 0.0));                        \
        tex000 = lerp(tex100, tex000, g0.x);                                                                   \
        tex010 = lerp(tex110, tex010, g0.x);                                                                   \
        tex000 = lerp(tex010, tex000, g0.y);                                                                   \
        tex001 = lerp(tex101, tex001, g0.x);                                                                   \
        tex011 = lerp(tex111, tex011, g0.x);                                                                   \
        tex001 = lerp(tex011, tex001, g0.y);                                                                   \
        return lerp(tex001, tex000, g0.z);                                                                     \
    }                                                                                                          \
                                                                                                               \

#define SAMPLE_TRICUBIC(TVolume, coord) TVolume##SAMPLER_TRICUBIC(coord)
