#include "PrecompiledHeader.h"
#include "VulkanFrameBufferCache.h"

namespace PK
{
    VulkanFrameBufferCache::VulkanFrameBufferCache(VkDevice device, uint64_t pruneDelay) :
        m_device(device),
        m_frameBuffers(512),
        m_renderPasses(512),
        m_pruneDelay(pruneDelay)
    {
    }

    VulkanFrameBufferCache::~VulkanFrameBufferCache()
    {
        m_frameBufferPool.Clear();
        m_renderPassPool.Clear();
        m_frameBuffers.Clear();
        m_renderPasses.Clear();
    }

    const VulkanFrameBuffer* VulkanFrameBufferCache::GetFrameBuffer(const FrameBufferKey& key)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        auto index = 0u;
        FrameBufferValue* value = nullptr;

        if (!m_frameBuffers.AddKey(key, &index))
        {
            value = &m_frameBuffers.GetValueAt(index);
            value->pruneTick = nextPruneTick;
            return value->frameBuffer;
        }

        VkImageView attachments[PK_RHI_MAX_RENDER_TARGETS * 2 + 1];
        uint32_t attachmentCount = 0;

        for (auto attachment : key.color)
        {
            if (attachment)
            {
                attachments[attachmentCount++] = attachment;
            }
        }

        for (auto attachment : key.resolve)
        {
            if (attachment)
            {
                attachments[attachmentCount++] = attachment;
            }
        }

        if (key.depth)
        {
            attachments[attachmentCount++] = key.depth;
        }

        VkFramebufferCreateInfo info{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        info.renderPass = key.renderPass;
        info.attachmentCount = attachmentCount;
        info.pAttachments = attachments;
        info.width = key.extent.width;
        info.height = key.extent.height;
        info.layers = key.layers;

        value = &m_frameBuffers.GetValueAt(index);
        value->frameBuffer = m_frameBufferPool.New(m_device, info);
        value->pruneTick = nextPruneTick;
        m_renderPassReferenceCounts[key.renderPass]++;
        return value->frameBuffer;
    }

    const VulkanRenderPass* VulkanFrameBufferCache::GetRenderPass(const RenderPassKey& key)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        auto index = 0u;
        RenderPassValue* value = nullptr;

        if (!m_renderPasses.AddKey(key, &index))
        {
            value = &m_renderPasses.GetValueAt(index);
            value->pruneTick = nextPruneTick;
            return value->renderPass;
        }

        VkAttachmentReference colorAttachmentRefs[PK_RHI_MAX_RENDER_TARGETS] = {};
        VkAttachmentReference resolveAttachmentRef[PK_RHI_MAX_RENDER_TARGETS] = {};
        VkAttachmentReference depthAttachmentRef{};

        // Note that this needs to have the same ordering as the corollary array in GetFrameBuffer.
        VkAttachmentDescription attachments[PK_RHI_MAX_RENDER_TARGETS * 2 + 1] = {};

        const bool hasDepth = key.depth.format != VK_FORMAT_UNDEFINED;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pColorAttachments = colorAttachmentRefs;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentRef : nullptr;
        subpass.pResolveAttachments = resolveAttachmentRef;

        VkSubpassDependency dependencies[2]{};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].srcStageMask = key.stageMask;
        dependencies[0].srcAccessMask = key.accessMask;

        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = 0;
        dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        VkRenderPassCreateInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderPassInfo.attachmentCount = 0;
        renderPassInfo.pAttachments = attachments;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = key.dynamicTargets ? 2 : 1;
        renderPassInfo.pDependencies = dependencies;

        auto attachmentIndex = 0u;

        for (auto i = 0u; i < PK_RHI_MAX_RENDER_TARGETS; i++)
        {
            if (key.colors[i].format != VK_FORMAT_UNDEFINED)
            {
                uint32_t index = subpass.colorAttachmentCount++;
                colorAttachmentRefs[index].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorAttachmentRefs[index].attachment = attachmentIndex;

                auto* attachment = attachments + attachmentIndex++;
                attachment->format = key.colors[i].format;
                attachment->samples = key.samples;
                attachment->loadOp = VulkanEnumConvert::GetLoadOp(key.colors[i].initialLayout, key.colors[i].loadop);
                attachment->storeOp = VulkanEnumConvert::GetStoreOp(key.colors[i].storeop);
                attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment->initialLayout = key.colors[i].loadop == LoadOp::Keep ? key.colors[i].initialLayout : VK_IMAGE_LAYOUT_UNDEFINED;
                attachment->finalLayout = key.colors[i].finalLayout;
            }
        }

        VkAttachmentReference* pResolveAttachment = resolveAttachmentRef;
        for (uint32_t i = 0u; i < PK_RHI_MAX_RENDER_TARGETS; ++i)
        {
            if (key.colors[i].format == VK_FORMAT_UNDEFINED)
            {
                continue;
            }

            if (!key.colors[i].resolve)
            {
                pResolveAttachment->attachment = VK_ATTACHMENT_UNUSED;
                ++pResolveAttachment;
                continue;
            }

            pResolveAttachment->attachment = attachmentIndex;
            pResolveAttachment->layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            ++pResolveAttachment;

            auto* attachment = attachments + attachmentIndex++;
            attachment->format = key.colors[i].format;
            attachment->samples = VK_SAMPLE_COUNT_1_BIT;
            attachment->loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment->storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            attachment->finalLayout = key.colors[i].finalLayout;
        }

        if (hasDepth)
        {
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthAttachmentRef.attachment = attachmentIndex;

            auto* attachment = attachments + attachmentIndex++;
            attachment->format = key.depth.format;
            attachment->samples = key.samples;
            attachment->loadOp = VulkanEnumConvert::GetLoadOp(key.depth.initialLayout, key.depth.loadop);
            attachment->storeOp = VulkanEnumConvert::GetStoreOp(key.depth.storeop);
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment->initialLayout = key.depth.loadop == LoadOp::Keep ? key.depth.initialLayout : VK_IMAGE_LAYOUT_UNDEFINED;
            attachment->finalLayout = key.depth.finalLayout;
            dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        renderPassInfo.attachmentCount = attachmentIndex;

        value = &m_renderPasses.GetValueAt(index);
        value->renderPass = m_renderPassPool.New(m_device, renderPassInfo);
        value->pruneTick = nextPruneTick;
        return value->renderPass;
    }

    void VulkanFrameBufferCache::Prune()
    {
        m_currentPruneTick++;

        for (int32_t i = m_frameBuffers.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_frameBuffers.GetValueAt(i);

            if (value->pruneTick < m_currentPruneTick)
            {
                m_renderPassReferenceCounts[m_frameBuffers.GetKeyAt(i).renderPass]--;
                m_frameBufferPool.Delete(value->frameBuffer);
                m_frameBuffers.RemoveAt(i);
            }
        }

        for (int32_t i = m_renderPasses.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_renderPasses.GetValueAt(i);

            if (value->pruneTick < m_currentPruneTick && m_renderPassReferenceCounts[value->renderPass->renderPass] == 0u)
            {
                m_renderPassReferenceCounts.erase(value->renderPass->renderPass);
                m_renderPassPool.Delete(value->renderPass);
                m_renderPasses.RemoveAt(i);
            }
        }
    }
}