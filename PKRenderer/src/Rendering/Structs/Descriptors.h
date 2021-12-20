#pragma once
#include "PrecompiledHeader.h"
#include "Math/PKMath.h"
#include "Enums.h"

namespace PK::Rendering::Structs
{
    using namespace PK::Math;

    struct SamplerDescriptor
    {
        FilterMode filter = FilterMode::Point;
        WrapMode wrap[3] = { WrapMode::Clamp, WrapMode::Clamp, WrapMode::Clamp };
        Comparison comparison = Comparison::Off;
        BorderColor borderColor = BorderColor::FloatClear;
        bool normalized = true;
        float anisotropy = 0.0f;
        float mipBias = 0.0f;
        float mipMin = 0.0f;
        float mipMax = 0.0f;
    
        inline bool operator < (const SamplerDescriptor& other) const noexcept
        {
            return memcmp(this, &other, sizeof(SamplerDescriptor)) < 0;
        }
    };


    struct TextureDescriptor
    {
        TextureFormat format = TextureFormat::RGBA8;
        SamplerType samplerType = SamplerType::Sampler2D;
        TextureUsage usage = TextureUsage::Default;
        uint3 resolution = PK_UINT3_ONE;
        uint8_t levels = 1;
        uint8_t samples = 1;
        uint16_t layers = 1;
        SamplerDescriptor sampler = {};
    };

    struct RenderTextureDescriptor
    {
        TextureFormat colorFormats[PK_MAX_RENDER_TARGETS] = { TextureFormat::RGBA8 };
        TextureFormat depthFormat = TextureFormat::Depth24_Stencil8;
        SamplerType samplerType = SamplerType::Sampler2D;
        TextureUsage usage = TextureUsage::Default;
        uint3 resolution = PK_UINT3_ONE;
        uint8_t levels = 1;
        uint8_t samples = 1;
        uint16_t layers = 1;
        SamplerDescriptor sampler = {};
    };

    struct TextureViewRange
    {
        ushort level = 0u;
        ushort layer = 0u;
        ushort levels = 0u;
        ushort layers = 0u;
    };

    struct MultisamplingParameters
    {
        uint rasterizationSamples = 1;
        bool sampleShadingEnable = false;
        bool alphaToCoverageEnable = false;
        bool alphaToOneEnable = false;
        float minSampleShading = 1.0f;
    };

    struct DepthStencilParameters
    {
        Comparison depthCompareOp = Comparison::Always;
        bool depthWriteEnable = false;
        bool depthBoundsTestEnable = false;
        bool stencilTestEnable = false;
        float minDepthBounds = 0.0f;
        float maxDepthBounds = 1.0f;
    };

    struct RasterizationParameters
    {
        bool depthClampEnable = false;
        bool rasterizerDiscardEnable = false;
        bool depthBiasEnable = false;
        PolygonMode polygonMode = PolygonMode::Fill;
        CullMode cullMode = CullMode::Off;
        FrontFace frontFace = FrontFace::CounterClockwise;
        float depthBiasConstantFactor = 0;
        float depthBiasClamp = 0;
        float depthBiasSlopeFactor = 0;
        float lineWidth = 1.0f;
    };

    struct BlendParameters
    {
        BlendFactor srcColorFactor;
        BlendFactor dstColorFactor;
        BlendOp colorOp;
        BlendFactor srcAlphaFactor;
        BlendFactor dstAlphaFactor;
        BlendOp alphaOp;
        ColorMask colorMask;

        inline bool isBlendEnabled() const { return colorOp != BlendOp::None || alphaOp != BlendOp::None ||
                                                    srcColorFactor != BlendFactor::None || 
                                                    dstColorFactor != BlendFactor::None || 
                                                    srcAlphaFactor != BlendFactor::None || 
                                                    dstAlphaFactor != BlendFactor::None; }
    };

    // Limited set of values that can be defined in the shader source
    struct FixedFunctionShaderAttributes
    {
        BlendParameters blending{};
        DepthStencilParameters depthStencil{};
        RasterizationParameters rasterization{};
    };

    struct FixedFunctionState
    {
        RasterizationParameters rasterization{};
        BlendParameters blending{};
        DepthStencilParameters depthStencil{};
        MultisamplingParameters multisampling{};
        uint32_t colorTargetCount = 0;
        uint32_t viewportCount = 1;
    };
}