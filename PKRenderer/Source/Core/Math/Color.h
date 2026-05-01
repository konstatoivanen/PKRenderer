#pragma once
#include "Vector.h"

namespace PK::math
{
    template<typename T> vector<T,4> hexToRgb(uint32_t hex)
    {
        return vector<T,4>(
            static_cast<T>(((hex >> 24u) & 0xFFu) / 255.0f), 
            static_cast<T>(((hex >> 16u) & 0xFFu) / 255.0f),
            static_cast<T>(((hex >> 8u) & 0xFFu) / 255.0f),
            static_cast<T>(1));
    }

    template<typename T> vector<uint8_t,4> colorTo32(const vector<T,4>& color)
    {
        return vector<uint8_t,4>(color.r * static_cast<T>(255), color.g * static_cast<T>(255), color.b * static_cast<T>(255), color.a * static_cast<T>(255));
    }

    template<typename T> vector<T,4> rgbToHsv(const vector<T,4>& c)
    {
        const auto k = vector<T,4>(static_cast<T>(0.0), static_cast<T>(-1.0 / 3.0), static_cast<T>(2.0 / 3.0), static_cast<T>(-1.0));
        const auto p = lerp(vector<T,4>(c.bg, k.wz), vector<T,4>(c.gb, k.xy), step(c.b, c.g));
        const auto q = lerp(vector<T,4>(p.xyw, c.r), vector<T,4>(c.r, p.yzx), step(p.x, c.r));
        const auto d = q.x - min(q.w, q.y);
        const auto e = static_cast<T>(1.0e-10f);
        return vector<T,4>(abs(q.z + (q.w - q.y) / (static_cast<T>(6.0) * d + e)), d / (q.x + e), q.x, c.a);
    }

    template<typename T> vector<T,4> hsvToRgb(float3 c)
    {
        const auto k = vector<T,4>(static_cast<T>(1.0), static_cast<T>(2.0 / 3.0), static_cast<T>(1.0 / 3.0), static_cast<T>(3.0));
        const auto p = abs(frac(c.xxx + k.xyz) * static_cast<T>(6.0) - k.www);
        return vector<T,4>(c.zzz * lerp(k.xxx, saturate(p - k.xxx), c.y), static_cast<T>(1));
    }

    template<typename T> vector<T,4> hsvToRgb(T hue, T saturation, T value) 
    { 
        return hsvtorgb(vector<T,3>(hue, saturation, value)); 
    }

    template<typename T> vector<T,4> hueToRgb(T hue)
    {
        const auto R = abs(hue * static_cast<T>(6) - static_cast<T>(3)) - static_cast<T>(1);
        const auto G = static_cast<T>(2) - abs(hue * static_cast<T>(6) - static_cast<T>(2));
        const auto B = static_cast<T>(2) - abs(hue * static_cast<T>(6) - static_cast<T>(4));
        return vector<T,4>(saturate(vector<T,3>(R,G,B)), static_cast<T>(1));
    }

    template<typename T> vector<uint8_t,4> hsvToRgb32(T hue) 
    { 
        return colorto32(hsvToRgb(hue)); 
    }

    template<typename T> vector<uint8_t, 4> hueToRgb32(T hue)
    {
        return colorTo32(hueToRgb(hue));
    }

    template<typename T> vector<T,4> normalizeColor(const vector<T,4>& c)
    {
        const auto sum = (c.r + c.g + c.b) / static_cast<T>(3);
        return sum < static_cast<T>(FLT_EPSILON) ? vector<T,4>(static_cast<T>(1)) : color(c.r / sum, c.g / sum, c.b / sum, static_cast<T>(1));
    }

    // An analytical model of chromaticity of the standard illuminant, by Judd et al.
    // http://en.wikipedia.org/wiki/Standard_illuminant#Illuminant_series_D
    // Slightly modifed to adjust it with the D65 white point (x=0.31271, y=0.32902).
    template<typename T> T standardIlluminantY(T x)
    {
        return static_cast<T>(2.87) * x - static_cast<T>(3.0) * x * x - static_cast<T>(0.27509507);
    }

    // CIE xy chromaticity to CAT02 LMS.
    // http://en.wikipedia.org/wiki/LMS_color_space#CAT02
    template<typename T> vector<T,3> CIExyToLMS(T x, T y)
    {
        const auto Y = static_cast<T>(1.0);
        const auto X = Y * x / y;
        const auto Z = Y * (static_cast<T>(1.0) - x - y) / y;
        const auto L = static_cast<T>(0.7328) * X + static_cast<T>(0.4296) * Y - static_cast<T>(0.1624) * Z;
        const auto M = static_cast<T>(-0.7036) * X + static_cast<T>(1.6975) * Y + static_cast<T>(0.0061) * Z;
        const auto S = static_cast<T>(0.0030) * X + static_cast<T>(0.0136) * Y + static_cast<T>(0.9834) * Z;
        return vector<T,3>(L, M, S);
    }

    template<typename T> vector<T,4> whiteBalance(T temperatureShift, T tint)
    {
        auto t1 = temperatureShift;
        auto t2 = tint;
        // Get the CIE xy chromaticity of the reference white point.
        // Note: 0.31271 = x value on the D65 white point
        auto x = static_cast<T>(0.31271) - t1 * (t1 < static_cast<T>(0) ? static_cast<T>(0.1) : static_cast<T>(0.05));
        auto y = standardIlluminantY(x) + t2 * static_cast<T>(0.05);
        // Calculate the coefficients in the LMS space.
        auto w1 = vector<T,3>(static_cast<T>(0.949237), static_cast<T>(1.03542), static_cast<T>(1.08728)); // D65 white point
        auto w2 = CIExyToLMS(x, y);
        return vector<T,4>(w1.x / w2.x, w1.y / w2.y, w1.z / w2.z, static_cast<T>(1));
    }

    template<typename T> void generateLiftGammaGain(
        const vector<T,4>& shadows, 
        const vector<T,4>& midtones, 
        const vector<T,4>& highlights, 
        vector<T,4>* outLift, 
        vector<T,4>* outGamma, 
        vector<T,4>* outGain)
    {
        const auto nLift = normalizeColor(shadows);
        const auto nGamma = normalizeColor(midtones);
        const auto nGain = normalizeColor(highlights);
        const auto avgLift = (nLift.r + nLift.g + nLift.b) / static_cast<T>(3.0);
        const auto avgGamma = (nGamma.r + nGamma.g + nGamma.b) / static_cast<T>(3.0);
        const auto avgGain = (nGain.r + nGain.g + nGain.b) / static_cast<T>(3.0);

        // Magic numbers
        const auto liftScale = static_cast<T>(0.1);
        const auto gammaScale = static_cast<T>(0.5);
        const auto gainScale = static_cast<T>(0.5);
        const auto minGamma = static_cast<T>(0.01);
        const auto liftR = (nLift.r - avgLift) * liftScale;
        const auto liftG = (nLift.g - avgLift) * liftScale;
        const auto liftB = (nLift.b - avgLift) * liftScale;
        const auto gammaR = pow(static_cast<T>(2.0), (nGamma.r - avgGamma) * gammaScale);
        const auto gammaG = pow(static_cast<T>(2.0), (nGamma.g - avgGamma) * gammaScale);
        const auto gammaB = pow(static_cast<T>(2.0), (nGamma.b - avgGamma) * gammaScale);
        const auto gainR = pow(static_cast<T>(2.0), (nGain.r - avgGain) * gainScale);
        const auto gainG = pow(static_cast<T>(2.0), (nGain.g - avgGain) * gainScale);
        const auto gainB = pow(static_cast<T>(2.0), (nGain.b - avgGain) * gainScale);
        const auto invGammaR = rcp(max(minGamma, gammaR));
        const auto invGammaG = rcp(max(minGamma, gammaG));
        const auto invGammaB = rcp(max(minGamma, gammaB));
        *outLift = vector<T,4>(liftR, liftG, liftB, static_cast<T>(1));
        *outGamma = vector<T,4>(invGammaR, invGammaG, invGammaB, static_cast<T>(1));
        *outGain = vector<T,4>(gainR, gainG, gainB, static_cast<T>(1));
    }

    template<typename T> T luminanceRec709(const vector<T,3>& color)
    {
        return dot(vector<T,3>(static_cast<T>(0.2126729), static_cast<T>(0.7151522), static_cast<T>(0.0721750)), color.rgb);
    }

    template<typename T> T linearToPerceptual(const vector<T,3>& color)
    {
        return log(max(luminanceRec709(color), static_cast<T>(0.001)));
    }

    template<typename T> T perceptualToLinear(T value)
    {
        return exp(value);
    }
}
