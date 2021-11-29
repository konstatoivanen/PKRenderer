#include "PrecompiledHeader.h"
#include "VulkanCommandBuffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    void VulkanCommandBuffer::BeginRenderPass()
    {
        renderState.PrepareRenderPass();
        VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
        renderPassInfo.renderPass = renderState.renderPass->renderPass;
        renderPassInfo.framebuffer = renderState.frameBuffer->frameBuffer;
        renderPassInfo.renderArea = renderState.renderArea;
        renderPassInfo.clearValueCount = renderState.clearValueCount;
        renderPassInfo.pClearValues = renderState.clearValues;
        BeginRenderPass(renderPassInfo);
    }

    void VulkanCommandBuffer::ValidatePipeline()
    {
        auto flags = renderState.ValidatePipeline(GetOnCompleteGate());

        if ((flags & PK_RENDER_STATE_DIRTY_PIPELINE) != 0)
        {
            auto pipeline = renderState.pipeline;
            auto bindPoint = EnumConvert::GetPipelineBindPoint(renderState.pipelineKey.shader->GetType());
            BindPipeline(bindPoint, pipeline->pipeline);
        }

        if ((flags & PK_RENDER_STATE_DIRTY_VERTEXBUFFERS) != 0)
        {
            BindVertexBuffers(0, renderState.vertexBufferBindCount, renderState.vertexBuffersRaw, renderState.vertexBufferOffsets);
        }

        for (auto i = 0; i < PK_MAX_DESCRIPTOR_SETS; ++i)
        {
            if ((flags & (PK_RENDER_STATE_DIRTY_DESCRIPTOR_SET_0 << i)) != 0)
            {
                auto pipeline = renderState.pipeline;
                auto bindPoint = EnumConvert::GetPipelineBindPoint(renderState.pipelineKey.shader->GetType());
                BindDescriptorSets(bindPoint, renderState.pipelineKey.shader->GetPipelineLayout(), 0, 1, &renderState.descriptorSets[i], 0, nullptr);
            }
        }
    }

    void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
    {
        ValidatePipeline();
        vkCmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }

    void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
    {
        ValidatePipeline();
        vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}