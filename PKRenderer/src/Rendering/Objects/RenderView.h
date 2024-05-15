#pragma once
#include "Core/TimeFrameInfo.h"
#include "Rendering/RHI/RHI.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/EntityEnums.h"

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

        Math::uint3 GetResolution() const;
        float GetAspectRatio() const;
        View GetView();
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

    struct RenderView : public Utilities::NoCopy
    {
        ConstantBufferRef constants = nullptr;
        GBuffersFull gbuffers{};

        RenderViewType type = RenderViewType::Scene;
        bool isWindowTarget = false;

        uint32_t viewEntityId = 0u;
        uint32_t primaryPassGroup;

        Math::uint4 renderAreaRect;
        Math::uint4 finalViewRect;

        Math::float4x4 worldToView;
        Math::float4x4 viewToClip;
        Math::float4x4 worldToClip;
        Math::float4 forwardPlane;
        float znear;
        float zfar;

        Core::TimeFrameInfo timeRender;
        Core::TimeFrameInfo timeResize;

        inline Math::uint3 GetResolution() const { return gbuffers.GetResolution(); }
        inline GBuffersFull::View GetGBuffersFullView() { return gbuffers.GetView(); }
    };
}