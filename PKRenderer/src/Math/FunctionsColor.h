#pragma once
#include "Types.h"

namespace PK::Math::Functions
{
    inline color HexToRGB(uint hex) { return color((hex >> 24) & 0xFF, (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, 255.0f) / 255.0f; }
    color HueToRGB(float hue);

    color NormalizeColor(const color& color);
    // CIE xy chromaticity to CAT02 LMS.
    // http://en.wikipedia.org/wiki/LMS_color_space#CAT02
    float3 CIExyToLMS(float x, float y);
    float4 GetWhiteBalance(float temperatureShift, float tint);
    void GenerateLiftGammaGain(const color& shadows, const color& midtones, const color& highlights, color* outLift, color* outGamma, color* outGain);
    // An analytical model of chromaticity of the standard illuminant, by Judd et al.
    // http://en.wikipedia.org/wiki/Standard_illuminant#Illuminant_series_D
    // Slightly modifed to adjust it with the D65 white point (x=0.31271, y=0.32902).
    inline float StandardIlluminantY(float x) { return 2.87f * x - 3.0f * x * x - 0.27509507f; }
    inline float Luminance(const color& color) { return glm::dot(float3(0.22f, 0.707f, 0.071f), float3(color.rgb)); }
    inline float LinearToPerceptual(const color& color) { return glm::log(glm::max(Luminance(color), 0.001f)); }
    inline float PerceptualToLinear(float value) { return glm::exp(value); }
}