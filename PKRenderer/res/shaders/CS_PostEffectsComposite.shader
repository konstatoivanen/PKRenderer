#version 450
#pragma PROGRAM_COMPUTE
#include includes/ColorGrading.glsl
#include includes/SharedFilmGrain.glsl
#include includes/SharedBloom.glsl
#include includes/SharedHistogram.glsl
#include includes/Common.glsl

layout(rgba16f, set = PK_SET_DRAW) uniform image2D _MainTex;

layout(local_size_x = 16, local_size_y = 4, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy);
    int2 size = imageSize(_MainTex).xy;

    if (Any_Greater(coord, size))
    {
        return;
    }

    float2 uv = float2(coord + 0.5f.xx) / float2(size);
    float3 color = imageLoad(_MainTex, coord).rgb;
    color = max(0.0f.xxx, color);

    float exposure = GetAutoExposure();
    exposure *= Vignette(uv);

    color = Bloom(color, uv);
    // Applying a bit of desaturation to reduce high intensity value color blowout
    // A personal preference really (should probably try to deprecate this).
    color = Saturation(color, 0.8f);
    color = TonemapACESFilm(color, exposure);
    color = FilmGrain(color, float2(coord));
    color = LinearToGamma(color);
    // This should perhaps be done before gamma corretion.
    // But doing so invalidates configurations done using external tools.
    color = ApplyColorGrading(color);

    imageStore(_MainTex, coord, float4(color, 1.0f));
}