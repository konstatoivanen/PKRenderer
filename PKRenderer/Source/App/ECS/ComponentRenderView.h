#pragma once
#include "Core/Math/Math.h"
#include "Core/Utilities/FixedString.h"
#include "App/Renderer/EntityEnums.h"

namespace PK::App
{
    // Reference to renderpipeline side representation
    struct ComponentRenderView
    {
        // These are forwarded to rendering side representation on execution
        FixedString16 name;
        uint4 desiredRect = PK_UINT4_MAX;
        // should the target be blit to the window
        bool isWindowTarget = true;

        struct RenderViewSettings* settingsRef = nullptr;
        struct RenderView* renderViewRef = nullptr;
        virtual ~ComponentRenderView() = default;
    };
}
