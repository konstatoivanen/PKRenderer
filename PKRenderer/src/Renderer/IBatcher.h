#pragma once
#include <cstdint>
#include "Utilities/ForwardDeclare.h"
#include "Graphics/GraphicsFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, struct ComponentTransform)

namespace PK::Renderer
{
    struct IBatcher
    {
        virtual ~IBatcher() = default;
        virtual void BeginCollectDrawCalls() = 0;
        virtual void EndCollectDrawCalls(Graphics::CommandBufferExt cmd) = 0;
        virtual uint32_t BeginNewGroup() = 0;
        virtual void SubmitMeshStaticDraw(ECS::ComponentTransform* transform,
            Graphics::Shader* shader,
            Graphics::Material* material,
            Graphics::MeshStatic* mesh,
            uint16_t submesh,
            uint32_t userdata,
            uint16_t sortDepth) = 0;

        virtual bool RenderGroup(Graphics::CommandBufferExt cmd,
            uint32_t group,
            Graphics::RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr,
            uint32_t requireKeyword = 0u) = 0;
    };
}