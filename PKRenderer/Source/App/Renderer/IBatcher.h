#pragma once
#include <cstdint>
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    struct ComponentTransform;

    struct IBatcher
    {
        virtual ~IBatcher() = default;
        virtual void BeginCollectDrawCalls() = 0;
        virtual void EndCollectDrawCalls(CommandBufferExt cmd) = 0;
        virtual uint32_t BeginNewGroup() = 0;
        virtual void SubmitMeshStaticDraw(ComponentTransform* transform,
            ShaderAsset* shader,
            Material* material,
            MeshStatic* mesh,
            uint16_t submesh,
            uint32_t userdata,
            uint16_t sortDepth) = 0;

        virtual bool RenderGroup(CommandBufferExt cmd,
            uint32_t group,
            FixedFunctionShaderAttributes* overrideAttributes = nullptr,
            uint32_t requireKeyword = 0u) = 0;
    };
}