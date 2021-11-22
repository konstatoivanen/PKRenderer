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
        Comparison comparison = Comparison::None;
        BorderColor borderColor = BorderColor::FloatClear;
        bool normalized = true;
        float anisotropy = 0.0f;
        float mipBias = 0.0f;
        float mipMin = 0.0f;
        float mipMax = 0.0f;
    
        inline bool operator < (const SamplerDescriptor& other) const noexcept
        {
            return memcmp(reinterpret_cast<const void*>(this), reinterpret_cast<const void*>(&other), sizeof(SamplerDescriptor)) < 0;
        }
    };


    struct TextureDescriptor
    {
        SamplerType samplerType = SamplerType::Sampler2D;
        TextureFormat format = TextureFormat::RGBA8;
        TextureUsage usage = TextureUsage::Default;
        uint3 resolution = PK_UINT3_ONE;
        uint8_t levels = 1;
        uint8_t samples = 1;
        uint16_t layers = 1;
        SamplerDescriptor sampler = {};
    };

}