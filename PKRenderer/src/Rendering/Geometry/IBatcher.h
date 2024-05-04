#pragma once
#include "Utilities/ForwardDeclare.h"
#include <cstdint>

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, struct ComponentTransform)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, class Material)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticMesh)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI, struct FixedFunctionShaderAttributes)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)

namespace PK::Rendering::Geometry
{
    struct IBatcher
    {
        virtual ~IBatcher() = default;
        virtual void BeginCollectDrawCalls() = 0;
        virtual void EndCollectDrawCalls(RHI::Objects::CommandBuffer* cmd) = 0;
        virtual uint32_t BeginNewGroup() = 0;
        virtual void SubmitStaticMeshDraw(ECS::ComponentTransform* transform,
            Rendering::RHI::Objects::Shader* shader,
            Rendering::Objects::Material* material,
            Rendering::Objects::StaticMesh* mesh,
            uint16_t submesh,
            uint32_t userdata,
            uint16_t sortDepth) = 0;

        virtual bool RenderGroup(Rendering::RHI::Objects::CommandBuffer* cmd,
            uint32_t group,
            Rendering::RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr,
            uint32_t requireKeyword = 0u) = 0;
    };
}