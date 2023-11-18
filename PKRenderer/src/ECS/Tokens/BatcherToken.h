#pragma once
#include "Math/Types.h"
#include "ECS/Components/Transform.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/StaticSceneMesh.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::ECS::Tokens
{
    struct IBatcher
    {
        virtual ~IBatcher() = default;
        virtual uint32_t BeginNewGroup() = 0;
        virtual void SubmitStaticDraw(ECS::Components::Transform* transform, 
                                      Rendering::RHI::Objects::Shader* shader, 
                                      Rendering::Objects::Material* material, 
                                      Rendering::Objects::StaticMesh* mesh, 
                                      uint16_t submesh,
                                      uint32_t userdata,
                                      uint16_t sortDepth) = 0;

        virtual bool Render(Rendering::RHI::Objects::CommandBuffer* cmd,
                            uint32_t group, 
                            Rendering::RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr, 
                            uint32_t requireKeyword = 0u) = 0;

        virtual bool RenderMeshlets(Rendering::RHI::Objects::CommandBuffer* cmd,
                                    uint32_t group,
                                    Rendering::RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr,
                                    uint32_t requireKeyword = 0u) = 0;
    };
}