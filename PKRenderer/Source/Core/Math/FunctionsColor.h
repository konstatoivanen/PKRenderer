#pragma once
#include "MathFwd.h"

namespace PK::Math
{
    color HexToRGB(uint32_t hex);
    color HueToRGB(float hue);
    color32 HueToRGB32(float hue);
    color NormalizeColor(const color& color);
    color32 ColorTo32(const color& color);
    // CIE xy chromaticity to CAT02 LMS.
    // http://en.wikipedia.org/wiki/LMS_color_space#CAT02
    float3 CIExyToLMS(float x, float y);
    float4 GetWhiteBalance(float temperatureShift, float tint);
    void GenerateLiftGammaGain(const color& shadows, const color& midtones, const color& highlights, color* outLift, color* outGamma, color* outGain);
    // An analytical model of chromaticity of the standard illuminant, by Judd et al.
    // http://en.wikipedia.org/wiki/Standard_illuminant#Illuminant_series_D
    // Slightly modifed to adjust it with the D65 white point (x=0.31271, y=0.32902).
    float StandardIlluminantY(float x);
    float Luminance(const color& color);
    float LinearToPerceptual(const color& color);
    float PerceptualToLinear(float value);
}
