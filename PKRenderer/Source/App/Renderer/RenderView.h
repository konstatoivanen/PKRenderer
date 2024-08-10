#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FixedArray.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/Rendering/RenderingFwd.h"
#include "App/Renderer/EntityEnums.h"
#include "App/Renderer/RenderViewSettings.h"

namespace PK::App
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

        struct TargetDescriptor
        {
            TextureFormat format;
            TextureUsage usage;
        };

        typedef FixedArray<TargetDescriptor, Count> Descriptor;

        struct View
        {
            RHITexture* color;
            RHITexture* normals;
            RHITexture* depthBiased;
            RHITexture* depth;
        };

        RHITextureRef color = nullptr;
        RHITextureRef normals = nullptr;
        RHITextureRef depthBiased = nullptr;
        RHITextureRef depth = nullptr;

        static uint2 AlignResolution(const uint2& resolution);
        uint3 GetResolution() const;
        float GetAspectRatio() const;
        View GetView();
        bool Validate(const uint2& resolution, const Descriptor& descriptor, const char* namePrefix);
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

        inline uint3 GetResolution() const { return current.GetResolution(); }
        inline float GetAspectRatio() const { return current.GetAspectRatio(); }
        inline View GetView() { return { current.GetView(), previous.GetView() }; }
        bool Validate(const uint2& resolution, const GBuffersFullDescriptor& descriptor, const char* namePrefix);
    };

    struct RenderView : public NoCopy
    {
        ConstantBufferRef constants = nullptr;
        GBuffersFull gbuffers{};
        RenderViewSettings settings{};

        RenderViewType type = RenderViewType::Scene;
        bool isWindowTarget = false;

        uint32_t viewEntityId = 0u;
        uint32_t primaryPassGroup;

        uint4 renderAreaRect;
        uint4 finalViewRect;
        uint2 bufferResolution;

        float4x4 worldToView;
        float4x4 viewToClip;
        float4x4 worldToClip;
        float4 forwardPlane;
        float znear;
        float zfar;

        TimeFrameInfo timeRender;
        TimeFrameInfo timeResize;

        inline uint3 GetResolution() const { return gbuffers.GetResolution(); }
        inline GBuffersFull::View GetGBuffersFullView() { return gbuffers.GetView(); }
    };
}