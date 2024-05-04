#pragma once
#include "Utilities/ForwardDeclare.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, class Mesh)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, struct CommandBuffer)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Buffer)

namespace PK::Rendering::Geometry
{
    void SetMesh(RHI::Objects::CommandBuffer* cmd, const PK::Rendering::Objects::Mesh* mesh);
    void DrawMesh(RHI::Objects::CommandBuffer* cmd, const PK::Rendering::Objects::Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance);
    void DrawMesh(RHI::Objects::CommandBuffer* cmd, const PK::Rendering::Objects::Mesh* mesh, int32_t submesh);
    void DrawMesh(RHI::Objects::CommandBuffer* cmd, const PK::Rendering::Objects::Mesh* mesh, int32_t submesh, const RHI::Objects::Shader* shader, int32_t variantIndex);
    void DrawMesh(RHI::Objects::CommandBuffer* cmd, const PK::Rendering::Objects::Mesh* mesh, int32_t submesh, const RHI::Objects::Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex);
    void DrawMeshIndirect(RHI::Objects::CommandBuffer* cmd, const PK::Rendering::Objects::Mesh* mesh, const RHI::Objects::Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride);
}