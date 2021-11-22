#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanRenderState.h"
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI;
    using namespace PK::Utilities;

    struct VulkanCommandBuffer : public VulkanRawCommandBuffer
    {
        VulkanCommandBuffer() {}

        using VulkanRawCommandBuffer::BeginRenderPass;
        using VulkanRawCommandBuffer::BindVertexBuffers;
        using VulkanRawCommandBuffer::BindIndexBuffer;

        VulkanRenderState renderState;
        Ref<VulkanFence> fence = nullptr;

        inline void SetRenderTarget(const VulkanRenderTarget& renderTarget, uint32_t index) { renderState.SetRenderTarget(renderTarget, index); }
        inline void SetResolveTarget(const VulkanRenderTarget& renderTarget, uint32_t index) { renderState.SetResolveTarget(renderTarget, index); }
        inline void ClearColor(const color& color, uint32_t index) { renderState.ClearColor(color, index); }
        inline void ClearDepth(float depth, uint32_t stencil) { renderState.ClearDepth(depth, stencil); }
        inline void DiscardColor(uint32_t index) { renderState.DiscardColor(index); }
        inline void DiscardDepth() { renderState.DiscardDepth(); }

        inline void SetShader(const VulkanShader* shader) { renderState.SetShader(shader); }
        inline void BindVertexBuffers(const std::initializer_list<std::pair<const VulkanBindHandle*, InputRate>>& handles) { renderState.BindVertexBuffers(handles); }
        inline void BindIndexBuffer(const VulkanBindHandle* handle, size_t offset) { BindIndexBuffer(handle->buffer, offset, EnumConvert::GetIndexType(handle->bufferLayout->begin()->Type)); }
        inline void BindResource(uint32_t nameHashId, const VulkanBindHandle* handle) { renderState.BindResource(nameHashId, handle); }
        inline void BindResource(const char* name, const VulkanBindHandle* handle) { BindResource(StringHashID::StringToID(name), handle); }

        void BeginRenderPass();
        void ValidatePipeline();

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);
    };
}