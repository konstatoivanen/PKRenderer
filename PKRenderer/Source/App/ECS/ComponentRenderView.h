#pragma once
#include "App/Renderer/EntityEnums.h"
#include "App/Renderer/RenderViewSettings.h"

namespace PK::App
{
    struct RenderView;

    // Reference to renderpipeline side representation
    struct ComponentRenderView
    {
        // These are forwarded to rendering side representation on execution
        RenderViewType type;
        uint4 desiredRect = PK_UINT4_MAX;
        // should the target be blit to the window
        bool isWindowTarget = true;

        RenderViewSettings settings;

        RenderView* renderViewRef = nullptr;
        virtual ~ComponentRenderView() = default;
    };
}