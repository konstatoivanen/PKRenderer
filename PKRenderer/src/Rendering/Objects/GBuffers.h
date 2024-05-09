#pragma once
#include <stdexcept>
#include "Rendering/RHI/Objects/Texture.h"

namespace PK::Rendering::Objects
{
    struct GBuffers
    {
        constexpr static uint32_t RESOLUTION_ALIGNMENT = 32u;

        typedef enum
        {
            Color,
            Normals,
            DepthBiased,
            Depth,
            Count
        } Target;

        constexpr static const char* Names[Count] =
        {
            "Color",
            "Normals",
            "DepthBiased",
            "Depth"
        };

        struct Descriptor
        {
            RHI::TextureUsage usages[Count]{};
            RHI::TextureFormat formats[Count]{};
        };

        struct View
        {
            RHI::Objects::Texture* color;
            RHI::Objects::Texture* normals;
            RHI::Objects::Texture* depthBiased;
            RHI::Objects::Texture* depth;
        };

        RHI::Objects::TextureRef color = nullptr;
        RHI::Objects::TextureRef normals = nullptr;
        RHI::Objects::TextureRef depthBiased = nullptr;
        RHI::Objects::TextureRef depth = nullptr;

        inline Math::uint3 GetResolution() const { return color->GetResolution(); }
        inline float GetAspectRatio() const { return float(color->GetResolution().x) / float(color->GetResolution().y); }
        inline View GetView() { return { color.get(), normals.get(), depthBiased.get(), depth.get() }; }
        bool Validate(const Math::uint2& resolution, const Descriptor& descriptor, const char* namePrefix);
    };

    struct GBuffersFullDescriptor
    {
        GBuffers::Descriptor current{};
        GBuffers::Descriptor previous{};
    };

    struct GBuffersFull
    {
        struct View
        {
            GBuffers::View current;
            GBuffers::View previous;
        };

        GBuffers current;
        GBuffers previous;

        inline Math::uint3 GetResolution() const { return current.GetResolution(); }
        inline float GetAspectRatio() const { return current.GetAspectRatio(); }
        inline View GetView() { return { current.GetView(), previous.GetView() }; }
        bool Validate(const Math::uint2& resolution, const GBuffersFullDescriptor& descriptor, const char* namePrefix);
    };

}