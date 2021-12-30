#include "PrecompiledHeader.h"
#include "FunctionsColor.h"

namespace PK::Math::Functions
{
    color HueToRGB(float hue)
    {
        float R = abs(hue * 6 - 3) - 1;
        float G = 2 - abs(hue * 6 - 2);
        float B = 2 - abs(hue * 6 - 4);
        return float4(glm::clamp(float3(R, G, B), PK_FLOAT3_ZERO, PK_FLOAT3_ONE), 1.0f);
    }

    color NormalizeColor(const color& c)
    {
        auto sum = (c.r + c.g + c.b) / 3.0f;
        return sum < std::numeric_limits<float>().epsilon() ? PK_COLOR_WHITE : color(c.r / sum, c.g / sum, c.b / sum, 1.0f);
    }

    float3 CIExyToLMS(float x, float y)
    {
        auto Y = 1.0f;
        auto X = Y * x / y;
        auto Z = Y * (1.0f - x - y) / y;

        auto L = 0.7328f * X + 0.4296f * Y - 0.1624f * Z;
        auto M = -0.7036f * X + 1.6975f * Y + 0.0061f * Z;
        auto S = 0.0030f * X + 0.0136f * Y + 0.9834f * Z;

        return float3(L, M, S);
    }

    float4 GetWhiteBalance(float temperatureShift, float tint)
    {
        auto t1 = temperatureShift;
        auto t2 = tint;

        // Get the CIE xy chromaticity of the reference white point.
        // Note: 0.31271 = x value on the D65 white point
        auto x = 0.31271f - t1 * (t1 < 0.0f ? 0.1f : 0.05f);
        auto y = StandardIlluminantY(x) + t2 * 0.05f;

        // Calculate the coefficients in the LMS space.
        auto w1 = float3(0.949237f, 1.03542f, 1.08728f); // D65 white point
        auto w2 = CIExyToLMS(x, y);
        return float4(w1.x / w2.x, w1.y / w2.y, w1.z / w2.z, 1.0f);
    }

    void GenerateLiftGammaGain(const color& shadows, const color& midtones, const color& highlights, color* outLift, color* outGamma, color* outGain)
    {
        auto nLift = NormalizeColor(shadows);
        auto nGamma = NormalizeColor(midtones);
        auto nGain = NormalizeColor(highlights);

        auto avgLift = (nLift.r + nLift.g + nLift.b) / 3.0f;
        auto avgGamma = (nGamma.r + nGamma.g + nGamma.b) / 3.0f;
        auto avgGain = (nGain.r + nGain.g + nGain.b) / 3.0f;

        // Magic numbers
        const float liftScale = 0.1f;
        const float gammaScale = 0.5f;
        const float gainScale = 0.5f;

        auto liftR = (nLift.r - avgLift) * liftScale;
        auto liftG = (nLift.g - avgLift) * liftScale;
        auto liftB = (nLift.b - avgLift) * liftScale;

        auto gammaR = glm::pow(2.0f, (nGamma.r - avgGamma) * gammaScale);
        auto gammaG = glm::pow(2.0f, (nGamma.g - avgGamma) * gammaScale);
        auto gammaB = glm::pow(2.0f, (nGamma.b - avgGamma) * gammaScale);

        auto gainR = glm::pow(2.0f, (nGain.r - avgGain) * gainScale);
        auto gainG = glm::pow(2.0f, (nGain.g - avgGain) * gainScale);
        auto gainB = glm::pow(2.0f, (nGain.b - avgGain) * gainScale);

        const float minGamma = 0.01f;
        auto invGammaR = 1.0f / glm::max(minGamma, gammaR);
        auto invGammaG = 1.0f / glm::max(minGamma, gammaG);
        auto invGammaB = 1.0f / glm::max(minGamma, gammaB);

        *outLift = color(liftR, liftG, liftB, 1.0f);
        *outGamma = color(invGammaR, invGammaG, invGammaB, 1.0f);
        *outGain = color(gainR, gainG, gainB, 1.0f);
    }
}