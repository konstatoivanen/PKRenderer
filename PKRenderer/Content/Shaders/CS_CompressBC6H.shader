
#pragma pk_program SHADER_STAGE_COMPUTE main

#include "includes/Utilities.glsl"

PK_DECLARE_SET_SHADER uniform sampler2D pk_Texture;
PK_DECLARE_SET_SHADER uniform uimage2D pk_Image;

// Whether to use P2 modes (4 endpoints) for compression. Slow, but improves quality.
#define ENCODE_P2 (QUALITY == 1)

// Improve quality at small performance loss
#define INSET_COLOR_BBOX 1
#define OPTIMIZE_ENDPOINTS 1

// Whether to optimize for luminance error or for RGB error
#define LUMINANCE_WEIGHTS 1

#define HALF_MAX 65504.0f
#define PATTERN_NUM 32

uint f32tof16(float val)
{
    uint f32 = floatBitsToUint(val);
    uint f16 = 0u;
    uint sign = (f32 >> 16) & 0x8000u;
    int exponent = int((f32 >> 23) & 0xFFu) - 127;
    uint mantissa = f32 & 0x007FFFFFu;
    if (exponent == 128)
    {
        // Infinity or NaN\n"
        // NaN bits that are masked out by 0x3FF get discarded.\n"
        // This can turn some NaNs to infinity, but this is allowed by the spec.\n"
        f16 = sign | (0x1Fu << 10);
        f16 |= (mantissa & 0x3FFu);
    }
    else if (exponent > 15)
    {
        // Overflow - flush to Infinity\n"
        f16 = sign | (0x1Fu << 10);
    }
    else if (exponent > -15)
    {
        // Representable value
        exponent += 15;
        mantissa >>= 13;
        f16 = sign | uint(exponent << 10) | mantissa;
    }
    else
    {
        f16 = sign;
    }
    return f16;
}

float f16tof32(uint val)
{
    uint sign = (val & 0x8000u) << 16;
    int exponent = int((val & 0x7C00u) >> 10);
    uint mantissa = val & 0x03FFu;
    float f32 = 0.0;

    if (exponent == 0)
    {
        if (mantissa != 0u)
        {
            const float scale = 1.0 / (1 << 24);
            f32 = scale * mantissa;
        }
    }
    else if (exponent == 31)
    {
        return uintBitsToFloat(sign | 0x7F800000u | mantissa);
    }
    else
    {
        exponent -= 15;
        float scale;
        if (exponent < 0)
        {
            // The negative unary operator is buggy on OSX.\n"
            // Work around this by using abs instead.\n"
            scale = 1.0 / (1 << abs(exponent));
        }
        else
        {
            scale = 1 << exponent;
        }
        float decimal = 1.0 + float(mantissa) / float(1 << 10);
        f32 = scale * decimal;
    }

    if (sign != 0u)
    {
        f32 = -f32;
    }

    return f32;
}

uint2 f32tof16(float2 val) { return uint2(f32tof16(val.x), f32tof16(val.y)); }
uint3 f32tof16(float3 val) { return uint3(f32tof16(val.x), f32tof16(val.y), f32tof16(val.z)); }
uint4 f32tof16(float4 val) { return uint4(f32tof16(val.x), f32tof16(val.y), f32tof16(val.z), f32tof16(val.w)); }

float2 f16tof32(uint2 val) { return float2(f16tof32(val.x), f16tof32(val.y)); }
float3 f16tof32(uint3 val) { return float3(f16tof32(val.x), f16tof32(val.y), f16tof32(val.z)); }
float4 f16tof32(uint4 val) { return float4(f16tof32(val.x), f16tof32(val.y), f16tof32(val.z), f16tof32(val.w)); }

float CalcMSLE(float3 a, float3 b)
{
    float3 delta = log2((b + 1.0f) / (a + 1.0f));
    float3 deltaSq = delta * delta;

#if LUMINANCE_WEIGHTS
    float3 luminanceWeights = float3(0.299f, 0.587f, 0.114f);
    deltaSq *= luminanceWeights;
#endif

    return deltaSq.x + deltaSq.y + deltaSq.z;
}

uint PatternFixupID(uint i)
{
    uint ret = 15;
    ret = ((3441033216 >> i) & 0x1) != 0 ? 2 : ret;
    ret = ((845414400 >> i) & 0x1) != 0 ? 8 : ret;
    return ret;
}

uint Pattern(uint p, uint i)
{
    uint p2 = p / 2;
    uint p3 = p - p2 * 2;

    uint enc = 0;
    enc = p2 == 0 ? 2290666700 : enc;
    enc = p2 == 1 ? 3972591342 : enc;
    enc = p2 == 2 ? 4276930688 : enc;
    enc = p2 == 3 ? 3967876808 : enc;
    enc = p2 == 4 ? 4293707776 : enc;
    enc = p2 == 5 ? 3892379264 : enc;
    enc = p2 == 6 ? 4278255592 : enc;
    enc = p2 == 7 ? 4026597360 : enc;
    enc = p2 == 8 ? 9369360 : enc;
    enc = p2 == 9 ? 147747072 : enc;
    enc = p2 == 10 ? 1930428556 : enc;
    enc = p2 == 11 ? 2362323200 : enc;
    enc = p2 == 12 ? 823134348 : enc;
    enc = p2 == 13 ? 913073766 : enc;
    enc = p2 == 14 ? 267393000 : enc;
    enc = p2 == 15 ? 966553998 : enc;

    enc = p3 != 0 ? enc >> 16 : enc;
    uint ret = (enc >> i) & 0x1;
    return ret;
}

float3 Quantize7(float3 x)
{
    return (f32tof16(x) * 128.0f) / (0x7bff + 1.0f);
}

float3 Quantize9(float3 x)
{
    return (f32tof16(x) * 512.0f) / (0x7bff + 1.0f);
}

float3 Quantize10(float3 x)
{
    return (f32tof16(x) * 1024.0f) / (0x7bff + 1.0f);
}

float3 Unquantize7(float3 x)
{
    return (x * 65536.0f + 0x8000) / 128.0f;
}

float3 Unquantize9(float3 x)
{
    return (x * 65536.0f + 0x8000) / 512.0f;
}

float3 Unquantize10(float3 x)
{
    return (x * 65536.0f + 0x8000) / 1024.0f;
}

float3 FinishUnquantize(float3 endpoint0Unq, float3 endpoint1Unq, float weight)
{
    float3 comp = (endpoint0Unq * (64.0f - weight) + endpoint1Unq * weight + 32.0f) * (31.0f / 4096.0f);
    return f16tof32(uint3(comp));
}

void Swap(inout float3 a, inout float3 b)
{
    float3 tmp = a;
    a = b;
    b = tmp;
}

void Swap(inout float a, inout float b)
{
    float tmp = a;
    a = b;
    b = tmp;
}

uint ComputeIndex3(float texelPos, float endPoint0Pos, float endPoint1Pos)
{
    float r = (texelPos - endPoint0Pos) / (endPoint1Pos - endPoint0Pos);
    return uint(clamp(r * 6.98182f + 0.00909f + 0.5f, 0.0f, 7.0f));
}

uint ComputeIndex4(float texelPos, float endPoint0Pos, float endPoint1Pos)
{
    float r = (texelPos - endPoint0Pos) / (endPoint1Pos - endPoint0Pos);
    return uint(clamp(r * 14.93333f + 0.03333f + 0.5f, 0.0f, 15.0f));
}

void SignExtend(inout float3 v1, uint mask, uint signFlag)
{
    int3 v = int3(v1);
    v.x = int((v.x & mask) | (v.x < 0 ? signFlag : 0u));
    v.y = int((v.y & mask) | (v.y < 0 ? signFlag : 0u));
    v.z = int((v.z & mask) | (v.z < 0 ? signFlag : 0u));
    v1 = v;
}

// Refine endpoints by insetting bounding box in log2 RGB space
void InsetColorBBoxP1(float3 texels[16], inout float3 blockMin, inout float3 blockMax)
{
    float3 refinedBlockMin = blockMax;
    float3 refinedBlockMax = blockMin;

    for (uint i = 0; i < 16; ++i)
    {
        refinedBlockMin = min(refinedBlockMin, texels[i] == blockMin ? refinedBlockMin : texels[i]);
        refinedBlockMax = max(refinedBlockMax, texels[i] == blockMax ? refinedBlockMax : texels[i]);
    }

    float3 logRefinedBlockMax = log2(refinedBlockMax + 1.0f);
    float3 logRefinedBlockMin = log2(refinedBlockMin + 1.0f);

    float3 logBlockMax = log2(blockMax + 1.0f);
    float3 logBlockMin = log2(blockMin + 1.0f);
    float3 logBlockMaxExt = (logBlockMax - logBlockMin) * (1.0f / 32.0f);

    logBlockMin += min(logRefinedBlockMin - logBlockMin, logBlockMaxExt);
    logBlockMax -= min(logBlockMax - logRefinedBlockMax, logBlockMaxExt);

    blockMin = exp2(logBlockMin) - 1.0f;
    blockMax = exp2(logBlockMax) - 1.0f;
}

// Refine endpoints by insetting bounding box in log2 RGB space
void InsetColorBBoxP2(float3 texels[16], uint pattern, uint patternSelector, inout float3 blockMin, inout float3 blockMax)
{
    float3 refinedBlockMin = blockMax;
    float3 refinedBlockMax = blockMin;

    for (uint i = 0; i < 16; ++i)
    {
        uint paletteID = Pattern(pattern, i);
        if (paletteID == patternSelector)
        {
            refinedBlockMin = min(refinedBlockMin, texels[i] == blockMin ? refinedBlockMin : texels[i]);
            refinedBlockMax = max(refinedBlockMax, texels[i] == blockMax ? refinedBlockMax : texels[i]);
        }
    }

    float3 logRefinedBlockMax = log2(refinedBlockMax + 1.0f);
    float3 logRefinedBlockMin = log2(refinedBlockMin + 1.0f);

    float3 logBlockMax = log2(blockMax + 1.0f);
    float3 logBlockMin = log2(blockMin + 1.0f);
    float3 logBlockMaxExt = (logBlockMax - logBlockMin) * (1.0f / 32.0f);

    logBlockMin += min(logRefinedBlockMin - logBlockMin, logBlockMaxExt);
    logBlockMax -= min(logBlockMax - logRefinedBlockMax, logBlockMaxExt);

    blockMin = exp2(logBlockMin) - 1.0f;
    blockMax = exp2(logBlockMax) - 1.0f;
}

// Least squares optimization to find best endpoints for the selected block indices
void OptimizeEndpointsP1(float3 texels[16], inout float3 blockMin, inout float3 blockMax)
{
    float3 blockDir = blockMax - blockMin;
    blockDir = blockDir / (blockDir.x + blockDir.y + blockDir.z);

    float endPoint0Pos = f32tof16(dot(blockMin, blockDir));
    float endPoint1Pos = f32tof16(dot(blockMax, blockDir));

    float3 alphaTexelSum = 0.0f.xxx;
    float3 betaTexelSum = 0.0f.xxx;
    float alphaBetaSum = 0.0f;
    float alphaSqSum = 0.0f;
    float betaSqSum = 0.0f;

    for (int i = 0; i < 16; i++)
    {
        float texelPos = f32tof16(dot(texels[i], blockDir));
        uint texelIndex = ComputeIndex4(texelPos, endPoint0Pos, endPoint1Pos);

        float beta = saturate(texelIndex / 15.0f);
        float alpha = 1.0f - beta;

        float3 texelF16 = f32tof16(texels[i].xyz);
        alphaTexelSum += alpha * texelF16;
        betaTexelSum += beta * texelF16;

        alphaBetaSum += alpha * beta;

        alphaSqSum += alpha * alpha;
        betaSqSum += beta * beta;
    }

    float det = alphaSqSum * betaSqSum - alphaBetaSum * alphaBetaSum;

    if (abs(det) > 0.00001f)
    {
        float detRcp = 1.0f / det;
        blockMin = f16tof32(uint3(clamp(detRcp * (alphaTexelSum * betaSqSum - betaTexelSum * alphaBetaSum), 0.0f.xxx, HALF_MAX.xxx)));
        blockMax = f16tof32(uint3(clamp(detRcp * (betaTexelSum * alphaSqSum - alphaTexelSum * alphaBetaSum), 0.0f.xxx, HALF_MAX.xxx)));
    }
}

// Least squares optimization to find best endpoints for the selected block indices
void OptimizeEndpointsP2(float3 texels[16], uint pattern, uint patternSelector, inout float3 blockMin, inout float3 blockMax)
{
    float3 blockDir = blockMax - blockMin;
    blockDir = blockDir / (blockDir.x + blockDir.y + blockDir.z);

    float endPoint0Pos = f32tof16(dot(blockMin, blockDir));
    float endPoint1Pos = f32tof16(dot(blockMax, blockDir));

    float3 alphaTexelSum = 0.0f.xxx;
    float3 betaTexelSum = 0.0f.xxx;
    float alphaBetaSum = 0.0f;
    float alphaSqSum = 0.0f;
    float betaSqSum = 0.0f;

    for (int i = 0; i < 16; i++)
    {
        uint paletteID = Pattern(pattern, i);
        if (paletteID == patternSelector)
        {
            float texelPos = f32tof16(dot(texels[i], blockDir));
            uint texelIndex = ComputeIndex3(texelPos, endPoint0Pos, endPoint1Pos);

            float beta = saturate(texelIndex / 7.0f);
            float alpha = 1.0f - beta;

            float3 texelF16 = f32tof16(texels[i].xyz);
            alphaTexelSum += alpha * texelF16;
            betaTexelSum += beta * texelF16;

            alphaBetaSum += alpha * beta;

            alphaSqSum += alpha * alpha;
            betaSqSum += beta * beta;
        }
    }

    float det = alphaSqSum * betaSqSum - alphaBetaSum * alphaBetaSum;

    if (abs(det) > 0.00001f)
    {
        float detRcp = 1.0f / det;
        blockMin = f16tof32(uint3(clamp(detRcp * (alphaTexelSum * betaSqSum - betaTexelSum * alphaBetaSum), 0.0f.xxx, HALF_MAX.xxx)));
        blockMax = f16tof32(uint3(clamp(detRcp * (betaTexelSum * alphaSqSum - alphaTexelSum * alphaBetaSum), 0.0f.xxx, HALF_MAX.xxx)));
    }
}

void EncodeP1(inout uint4 block, inout float blockMSLE, float3 texels[16])
{
    // compute endpoints (min/max RGB bbox)
    float3 blockMin = texels[0];
    float3 blockMax = texels[0];
    for (uint i = 1; i < 16; ++i)
    {
        blockMin = min(blockMin, texels[i]);
        blockMax = max(blockMax, texels[i]);
    }

#if INSET_COLOR_BBOX
    InsetColorBBoxP1(texels, blockMin, blockMax);
#endif

#if OPTIMIZE_ENDPOINTS
    OptimizeEndpointsP1(texels, blockMin, blockMax);
#endif


    float3 blockDir = blockMax - blockMin;
    blockDir = blockDir / (blockDir.x + blockDir.y + blockDir.z);

    float3 endpoint0 = Quantize10(blockMin);
    float3 endpoint1 = Quantize10(blockMax);
    float endPoint0Pos = f32tof16(dot(blockMin, blockDir));
    float endPoint1Pos = f32tof16(dot(blockMax, blockDir));

    // check if endpoint swap is required
    float fixupTexelPos = f32tof16(dot(texels[0], blockDir));
    uint fixupIndex = ComputeIndex4(fixupTexelPos, endPoint0Pos, endPoint1Pos);
    if (fixupIndex > 7)
    {
        Swap(endPoint0Pos, endPoint1Pos);
        Swap(endpoint0, endpoint1);
    }

    // compute indices
    uint indices[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (uint i = 0; i < 16; ++i)
    {
        float texelPos = f32tof16(dot(texels[i], blockDir));
        indices[i] = ComputeIndex4(texelPos, endPoint0Pos, endPoint1Pos);
    }

    // compute compression error (MSLE)
    float3 endpoint0Unq = Unquantize10(endpoint0);
    float3 endpoint1Unq = Unquantize10(endpoint1);
    float msle = 0.0f;
    for (uint i = 0; i < 16; ++i)
    {
        float weight = floor((indices[i] * 64.0f) / 15.0f + 0.5f);
        float3 texelUnc = FinishUnquantize(endpoint0Unq, endpoint1Unq, weight);

        msle += CalcMSLE(texels[i], texelUnc);
    }


    // encode block for mode 11
    blockMSLE = msle;
    block.x = 0x03;

    // endpoints
    block.x |= uint(endpoint0.x) << 5;
    block.x |= uint(endpoint0.y) << 15;
    block.x |= uint(endpoint0.z) << 25;
    block.y |= uint(endpoint0.z) >> 7;
    block.y |= uint(endpoint1.x) << 3;
    block.y |= uint(endpoint1.y) << 13;
    block.y |= uint(endpoint1.z) << 23;
    block.z |= uint(endpoint1.z) >> 9;

    // indices
    block.z |= indices[0] << 1;
    block.z |= indices[1] << 4;
    block.z |= indices[2] << 8;
    block.z |= indices[3] << 12;
    block.z |= indices[4] << 16;
    block.z |= indices[5] << 20;
    block.z |= indices[6] << 24;
    block.z |= indices[7] << 28;
    block.w |= indices[8] << 0;
    block.w |= indices[9] << 4;
    block.w |= indices[10] << 8;
    block.w |= indices[11] << 12;
    block.w |= indices[12] << 16;
    block.w |= indices[13] << 20;
    block.w |= indices[14] << 24;
    block.w |= indices[15] << 28;
}

float DistToLineSq(float3 PointOnLine, float3 LineDirection, float3 Point)
{
    float3 w = Point - PointOnLine;
    float3 x = w - dot(w, LineDirection) * LineDirection;
    return dot(x, x);
}

// Evaluate how good is given P2 pattern for encoding current block
float EvaluateP2Pattern(int pattern, float3 texels[16])
{
    float3 p0BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
    float3 p0BlockMax = float3(0.0f, 0.0f, 0.0f);
    float3 p1BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
    float3 p1BlockMax = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < 16; ++i)
    {
        uint paletteID = Pattern(pattern, i);
        if (paletteID == 0)
        {
            p0BlockMin = min(p0BlockMin, texels[i]);
            p0BlockMax = max(p0BlockMax, texels[i]);
        }
        else
        {
            p1BlockMin = min(p1BlockMin, texels[i]);
            p1BlockMax = max(p1BlockMax, texels[i]);
        }
    }

    float3 p0BlockDir = normalize(p0BlockMax - p0BlockMin);
    float3 p1BlockDir = normalize(p1BlockMax - p1BlockMin);

    float sqDistanceFromLine = 0.0f;

    for (uint i = 0; i < 16; ++i)
    {
        uint paletteID = Pattern(pattern, i);
        if (paletteID == 0)
        {
            sqDistanceFromLine += DistToLineSq(p0BlockMin, p0BlockDir, texels[i]);
        }
        else
        {
            sqDistanceFromLine += DistToLineSq(p1BlockMin, p1BlockDir, texels[i]);
        }
    }

    return sqDistanceFromLine;
}

void EncodeP2Pattern(inout uint4 block, inout float blockMSLE, int pattern, float3 texels[16])
{
    float3 p0BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
    float3 p0BlockMax = float3(0.0f, 0.0f, 0.0f);
    float3 p1BlockMin = float3(HALF_MAX, HALF_MAX, HALF_MAX);
    float3 p1BlockMax = float3(0.0f, 0.0f, 0.0f);

    for (uint i = 0; i < 16; ++i)
    {
        uint paletteID = Pattern(pattern, i);
        if (paletteID == 0)
        {
            p0BlockMin = min(p0BlockMin, texels[i]);
            p0BlockMax = max(p0BlockMax, texels[i]);
        }
        else
        {
            p1BlockMin = min(p1BlockMin, texels[i]);
            p1BlockMax = max(p1BlockMax, texels[i]);
        }
    }

#if INSET_COLOR_BBOX
    // Disabled because it was a negligible quality increase
    //InsetColorBBoxP2(texels, pattern, 0, p0BlockMin, p0BlockMax);
    //InsetColorBBoxP2(texels, pattern, 1, p1BlockMin, p1BlockMax);
#endif

#if OPTIMIZE_ENDPOINTS
    OptimizeEndpointsP2(texels, pattern, 0, p0BlockMin, p0BlockMax);
    OptimizeEndpointsP2(texels, pattern, 1, p1BlockMin, p1BlockMax);
#endif

    float3 p0BlockDir = p0BlockMax - p0BlockMin;
    float3 p1BlockDir = p1BlockMax - p1BlockMin;
    p0BlockDir = p0BlockDir / (p0BlockDir.x + p0BlockDir.y + p0BlockDir.z);
    p1BlockDir = p1BlockDir / (p1BlockDir.x + p1BlockDir.y + p1BlockDir.z);


    float p0Endpoint0Pos = f32tof16(dot(p0BlockMin, p0BlockDir));
    float p0Endpoint1Pos = f32tof16(dot(p0BlockMax, p0BlockDir));
    float p1Endpoint0Pos = f32tof16(dot(p1BlockMin, p1BlockDir));
    float p1Endpoint1Pos = f32tof16(dot(p1BlockMax, p1BlockDir));


    uint fixupID = PatternFixupID(pattern);
    float p0FixupTexelPos = f32tof16(dot(texels[0], p0BlockDir));
    float p1FixupTexelPos = f32tof16(dot(texels[fixupID], p1BlockDir));
    uint p0FixupIndex = ComputeIndex3(p0FixupTexelPos, p0Endpoint0Pos, p0Endpoint1Pos);
    uint p1FixupIndex = ComputeIndex3(p1FixupTexelPos, p1Endpoint0Pos, p1Endpoint1Pos);
    if (p0FixupIndex > 3)
    {
        Swap(p0Endpoint0Pos, p0Endpoint1Pos);
        Swap(p0BlockMin, p0BlockMax);
    }
    if (p1FixupIndex > 3)
    {
        Swap(p1Endpoint0Pos, p1Endpoint1Pos);
        Swap(p1BlockMin, p1BlockMax);
    }

    uint indices[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (uint i = 0; i < 16; ++i)
    {
        float p0TexelPos = f32tof16(dot(texels[i], p0BlockDir));
        float p1TexelPos = f32tof16(dot(texels[i], p1BlockDir));
        uint p0Index = ComputeIndex3(p0TexelPos, p0Endpoint0Pos, p0Endpoint1Pos);
        uint p1Index = ComputeIndex3(p1TexelPos, p1Endpoint0Pos, p1Endpoint1Pos);

        uint paletteID = Pattern(pattern, i);
        indices[i] = paletteID == 0 ? p0Index : p1Index;
    }

    float3 endpoint760 = floor(Quantize7(p0BlockMin));
    float3 endpoint761 = floor(Quantize7(p0BlockMax));
    float3 endpoint762 = floor(Quantize7(p1BlockMin));
    float3 endpoint763 = floor(Quantize7(p1BlockMax));

    float3 endpoint950 = floor(Quantize9(p0BlockMin));
    float3 endpoint951 = floor(Quantize9(p0BlockMax));
    float3 endpoint952 = floor(Quantize9(p1BlockMin));
    float3 endpoint953 = floor(Quantize9(p1BlockMax));

    endpoint761 = endpoint761 - endpoint760;
    endpoint762 = endpoint762 - endpoint760;
    endpoint763 = endpoint763 - endpoint760;

    endpoint951 = endpoint951 - endpoint950;
    endpoint952 = endpoint952 - endpoint950;
    endpoint953 = endpoint953 - endpoint950;

    int maxVal76 = 0x1F;
    endpoint761 = clamp(endpoint761, -maxVal76, maxVal76);
    endpoint762 = clamp(endpoint762, -maxVal76, maxVal76);
    endpoint763 = clamp(endpoint763, -maxVal76, maxVal76);

    int maxVal95 = 0xF;
    endpoint951 = clamp(endpoint951, -maxVal95, maxVal95);
    endpoint952 = clamp(endpoint952, -maxVal95, maxVal95);
    endpoint953 = clamp(endpoint953, -maxVal95, maxVal95);

    float3 endpoint760Unq = Unquantize7(endpoint760);
    float3 endpoint761Unq = Unquantize7(endpoint760 + endpoint761);
    float3 endpoint762Unq = Unquantize7(endpoint760 + endpoint762);
    float3 endpoint763Unq = Unquantize7(endpoint760 + endpoint763);
    float3 endpoint950Unq = Unquantize9(endpoint950);
    float3 endpoint951Unq = Unquantize9(endpoint950 + endpoint951);
    float3 endpoint952Unq = Unquantize9(endpoint950 + endpoint952);
    float3 endpoint953Unq = Unquantize9(endpoint950 + endpoint953);

    float msle76 = 0.0f;
    float msle95 = 0.0f;
    for (uint i = 0; i < 16; ++i)
    {
        uint paletteID = Pattern(pattern, i);

        float3 tmp760Unq = paletteID == 0 ? endpoint760Unq : endpoint762Unq;
        float3 tmp761Unq = paletteID == 0 ? endpoint761Unq : endpoint763Unq;
        float3 tmp950Unq = paletteID == 0 ? endpoint950Unq : endpoint952Unq;
        float3 tmp951Unq = paletteID == 0 ? endpoint951Unq : endpoint953Unq;

        float weight = floor((indices[i] * 64.0f) / 7.0f + 0.5f);
        float3 texelUnc76 = FinishUnquantize(tmp760Unq, tmp761Unq, weight);
        float3 texelUnc95 = FinishUnquantize(tmp950Unq, tmp951Unq, weight);

        msle76 += CalcMSLE(texels[i], texelUnc76);
        msle95 += CalcMSLE(texels[i], texelUnc95);
    }

    SignExtend(endpoint761, 0x1F, 0x20);
    SignExtend(endpoint762, 0x1F, 0x20);
    SignExtend(endpoint763, 0x1F, 0x20);

    SignExtend(endpoint951, 0xF, 0x10);
    SignExtend(endpoint952, 0xF, 0x10);
    SignExtend(endpoint953, 0xF, 0x10);

    // encode block
    float p2MSLE = min(msle76, msle95);
    if (p2MSLE < blockMSLE)
    {
        blockMSLE = p2MSLE;
        block = uint4(0, 0, 0, 0);

        if (p2MSLE == msle76)
        {
            // 7.6
            block.x = 0x1;
            block.x |= (uint(endpoint762.y) & 0x20) >> 3;
            block.x |= (uint(endpoint763.y) & 0x10) >> 1;
            block.x |= (uint(endpoint763.y) & 0x20) >> 1;
            block.x |= uint(endpoint760.x) << 5;
            block.x |= (uint(endpoint763.z) & 0x01) << 12;
            block.x |= (uint(endpoint763.z) & 0x02) << 12;
            block.x |= (uint(endpoint762.z) & 0x10) << 10;
            block.x |= uint(endpoint760.y) << 15;
            block.x |= (uint(endpoint762.z) & 0x20) << 17;
            block.x |= (uint(endpoint763.z) & 0x04) << 21;
            block.x |= (uint(endpoint762.y) & 0x10) << 20;
            block.x |= uint(endpoint760.z) << 25;
            block.y |= (uint(endpoint763.z) & 0x08) >> 3;
            block.y |= (uint(endpoint763.z) & 0x20) >> 4;
            block.y |= (uint(endpoint763.z) & 0x10) >> 2;
            block.y |= uint(endpoint761.x) << 3;
            block.y |= (uint(endpoint762.y) & 0x0F) << 9;
            block.y |= uint(endpoint761.y) << 13;
            block.y |= (uint(endpoint763.y) & 0x0F) << 19;
            block.y |= uint(endpoint761.z) << 23;
            block.y |= (uint(endpoint762.z) & 0x07) << 29;
            block.z |= (uint(endpoint762.z) & 0x08) >> 3;
            block.z |= uint(endpoint762.x) << 1;
            block.z |= uint(endpoint763.x) << 7;
        }
        else
        {
            // 9.5
            block.x = 0xE;
            block.x |= uint(endpoint950.x) << 5;
            block.x |= (uint(endpoint952.z) & 0x10) << 10;
            block.x |= uint(endpoint950.y) << 15;
            block.x |= (uint(endpoint952.y) & 0x10) << 20;
            block.x |= uint(endpoint950.z) << 25;
            block.y |= uint(endpoint950.z) >> 7;
            block.y |= (uint(endpoint953.z) & 0x10) >> 2;
            block.y |= uint(endpoint951.x) << 3;
            block.y |= (uint(endpoint953.y) & 0x10) << 4;
            block.y |= (uint(endpoint952.y) & 0x0F) << 9;
            block.y |= uint(endpoint951.y) << 13;
            block.y |= (uint(endpoint953.z) & 0x01) << 18;
            block.y |= (uint(endpoint953.y) & 0x0F) << 19;
            block.y |= uint(endpoint951.z) << 23;
            block.y |= (uint(endpoint953.z) & 0x02) << 27;
            block.y |= uint(endpoint952.z) << 29;
            block.z |= (uint(endpoint952.z) & 0x08) >> 3;
            block.z |= uint(endpoint952.x) << 1;
            block.z |= (uint(endpoint953.z) & 0x04) << 4;
            block.z |= uint(endpoint953.x) << 7;
            block.z |= (uint(endpoint953.z) & 0x08) << 9;
        }

        block.z |= pattern << 13;
        uint blockFixupID = PatternFixupID(pattern);
        if (blockFixupID == 15)
        {
            block.z |= indices[0] << 18;
            block.z |= indices[1] << 20;
            block.z |= indices[2] << 23;
            block.z |= indices[3] << 26;
            block.z |= indices[4] << 29;
            block.w |= indices[5] << 0;
            block.w |= indices[6] << 3;
            block.w |= indices[7] << 6;
            block.w |= indices[8] << 9;
            block.w |= indices[9] << 12;
            block.w |= indices[10] << 15;
            block.w |= indices[11] << 18;
            block.w |= indices[12] << 21;
            block.w |= indices[13] << 24;
            block.w |= indices[14] << 27;
            block.w |= indices[15] << 30;
        }
        else if (blockFixupID == 2)
        {
            block.z |= indices[0] << 18;
            block.z |= indices[1] << 20;
            block.z |= indices[2] << 23;
            block.z |= indices[3] << 25;
            block.z |= indices[4] << 28;
            block.z |= indices[5] << 31;
            block.w |= indices[5] >> 1;
            block.w |= indices[6] << 2;
            block.w |= indices[7] << 5;
            block.w |= indices[8] << 8;
            block.w |= indices[9] << 11;
            block.w |= indices[10] << 14;
            block.w |= indices[11] << 17;
            block.w |= indices[12] << 20;
            block.w |= indices[13] << 23;
            block.w |= indices[14] << 26;
            block.w |= indices[15] << 29;
        }
        else
        {
            block.z |= indices[0] << 18;
            block.z |= indices[1] << 20;
            block.z |= indices[2] << 23;
            block.z |= indices[3] << 26;
            block.z |= indices[4] << 29;
            block.w |= indices[5] << 0;
            block.w |= indices[6] << 3;
            block.w |= indices[7] << 6;
            block.w |= indices[8] << 9;
            block.w |= indices[9] << 11;
            block.w |= indices[10] << 14;
            block.w |= indices[11] << 17;
            block.w |= indices[12] << 20;
            block.w |= indices[13] << 23;
            block.w |= indices[14] << 26;
            block.w |= indices[15] << 29;
        }
    }
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    uint2 blockCoord = gl_GlobalInvocationID.xy;
    uint2 levelSize = gl_NumWorkGroups.xy * gl_WorkGroupSize.xy;
    float2 texelSize = 1.0f / (4.0f.xx * levelSize);
    float2 uv = (blockCoord * 4.0f + texelSize) * texelSize;

    // Gather texels for current 4x4 block
    // 0 1 2 3
    // 4 5 6 7
    // 8 9 10 11
    // 12 13 14 15
    float2 block0UV = uv;
    float2 block1UV = uv + float2(2.0f * texelSize.x, 0.0f);
    float2 block2UV = uv + float2(0.0f, 2.0f * texelSize.y);
    float2 block3UV = uv + float2(2.0f * texelSize.x, 2.0f * texelSize.y);
    float4 block0X = textureGather(pk_Texture, block0UV, 0);
    float4 block1X = textureGather(pk_Texture, block1UV, 0);
    float4 block2X = textureGather(pk_Texture, block2UV, 0);
    float4 block3X = textureGather(pk_Texture, block3UV, 0);
    float4 block0Y = textureGather(pk_Texture, block0UV, 1);
    float4 block1Y = textureGather(pk_Texture, block1UV, 1);
    float4 block2Y = textureGather(pk_Texture, block2UV, 1);
    float4 block3Y = textureGather(pk_Texture, block3UV, 1);
    float4 block0Z = textureGather(pk_Texture, block0UV, 2);
    float4 block1Z = textureGather(pk_Texture, block1UV, 2);
    float4 block2Z = textureGather(pk_Texture, block2UV, 2);
    float4 block3Z = textureGather(pk_Texture, block3UV, 2);

    float3 texels[16];
    texels[0] = float3(block0X.w, block0Y.w, block0Z.w);
    texels[1] = float3(block0X.z, block0Y.z, block0Z.z);
    texels[2] = float3(block1X.w, block1Y.w, block1Z.w);
    texels[3] = float3(block1X.z, block1Y.z, block1Z.z);
    texels[4] = float3(block0X.x, block0Y.x, block0Z.x);
    texels[5] = float3(block0X.y, block0Y.y, block0Z.y);
    texels[6] = float3(block1X.x, block1Y.x, block1Z.x);
    texels[7] = float3(block1X.y, block1Y.y, block1Z.y);
    texels[8] = float3(block2X.w, block2Y.w, block2Z.w);
    texels[9] = float3(block2X.z, block2Y.z, block2Z.z);
    texels[10] = float3(block3X.w, block3Y.w, block3Z.w);
    texels[11] = float3(block3X.z, block3Y.z, block3Z.z);
    texels[12] = float3(block2X.x, block2Y.x, block2Z.x);
    texels[13] = float3(block2X.y, block2Y.y, block2Z.y);
    texels[14] = float3(block3X.x, block3Y.x, block3Z.x);
    texels[15] = float3(block3X.y, block3Y.y, block3Z.y);

    uint4 block = uint4(0, 0, 0, 0);
    float blockMSLE = 0.0f;

    EncodeP1(block, blockMSLE, texels);

#if ENCODE_P2
    // First find pattern which is a best fit for a current block
    float bestScore = EvaluateP2Pattern(0, texels);
    uint bestPattern = 0;

    for (uint patternIndex = 1; patternIndex < 32; ++patternIndex)
    {
        float score = EvaluateP2Pattern(patternIndex, texels);
        if (score < bestScore)
        {
            bestPattern = patternIndex;
            bestScore = score;
        }
    }

    // Then encode it
    EncodeP2Pattern(block, blockMSLE, bestPattern, texels);
#endif

    imageStore(pk_Image, int2(blockCoord), block);
}