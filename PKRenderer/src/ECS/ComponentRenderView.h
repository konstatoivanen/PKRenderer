#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Renderer/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Renderer, struct RenderView)

namespace PK::ECS
{
    // Reference to renderpipeline side representation
    struct ComponentRenderView
    {
        // These are forwarded to rendering side representation on execution
        Renderer::RenderViewType type;
        Math::uint4 desiredRect = Math::PK_UINT4_MAX;
        // should the target be blit to the window
        bool isWindowTarget = true;

        Renderer::RenderView* renderViewRef = nullptr;
        virtual ~ComponentRenderView() = default;
    };
}