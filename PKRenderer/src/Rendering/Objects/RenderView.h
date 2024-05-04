#pragma once
#include "Core/TimeFrameInfo.h"
#include "Rendering/Objects/ConstantBuffer.h"
#include "Rendering/Objects/GBuffers.h"
#include "Rendering/EntityEnums.h"

namespace PK::Rendering::Objects
{
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