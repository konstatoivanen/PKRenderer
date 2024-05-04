#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Rendering/EntityEnums.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct RenderView)

namespace PK::ECS
{
    // Reference to renderpipeline side representation
    struct ComponentRenderView
    {
        // These are forwarded to rendering side representation on execution
        Rendering::RenderViewType type;
        Math::uint4 desiredRect = Math::PK_UINT4_MAX;
        // should the target be blit to the window
        bool isWindowTarget = true;

        Rendering::Objects::RenderView* renderViewRef = nullptr;
        virtual ~ComponentRenderView() = default;
    };
}