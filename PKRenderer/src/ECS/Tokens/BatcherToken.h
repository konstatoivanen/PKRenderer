#pragma once
#include "Math/Types.h"
#include "ECS/Components/Transform.h"
#include "Rendering/Structs/Enums.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/Objects/Mesh.h"

namespace PK::ECS::Tokens
{
    struct IBatcher
    {
        virtual ~IBatcher() = default;
        virtual uint32_t BeginNewGroup() = 0;
        virtual void SubmitDraw(ECS::Components::Transform* transform, 
                                Rendering::Objects::Shader* shader, 
                                Rendering::Objects::Material* material, 
                                Rendering::Objects::Mesh* mesh, 
                                uint32_t submesh, 
                                uint32_t userdata) = 0;
        virtual bool Render(Rendering::Objects::CommandBuffer* cmd,
                            uint32_t group, 
                            Rendering::Structs::FixedFunctionShaderAttributes* overrideAttributes = nullptr, 
                            uint32_t requireKeyword = 0u) = 0;
    };
}