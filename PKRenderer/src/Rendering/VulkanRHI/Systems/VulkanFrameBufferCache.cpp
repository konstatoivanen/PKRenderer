#include "PrecompiledHeader.h"
#include "VulkanFrameBufferCache.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"

namespace PK::Rendering::VulkanRHI::Systems
{   
    using namespace Structs;

    VulkanFrameBufferCache::VulkanFrameBufferCache(VkDevice device, uint64_t pruneDelay) : m_device(device), m_pruneDelay(pruneDelay) {}

    VulkanFrameBufferCache::~VulkanFrameBufferCache()
    {
        for (auto& kv : m_framebuffers)
        {
            if (kv.second.frameBuffer != nullptr)
            {
                delete kv.second.frameBuffer;
            }
        }

        for (auto& kv : m_renderPasses)
        {
            if (kv.second.renderPass != nullptr)
            {
                delete kv.second.renderPass;
            }
        }
    }

    const VulkanFrameBuffer* VulkanFrameBufferCache::GetFrameBuffer(const FrameBufferKey& key)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        auto iterator = m_framebuffers.find(key);
        
        if (iterator != m_framebuffers.end() && iterator->second.frameBuffer != nullptr)
        {
            iterator->second.pruneTick = nextPruneTick;
            return iterator->second.frameBuffer;
        }

        VkImageView attachments[PK_MAX_RENDER_TARGETS * 2 + 1];
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

        auto frameBuffer = new VulkanFrameBuffer(m_device, info);
        m_framebuffers[key] = { frameBuffer, nextPruneTick };
        m_renderPassReferenceCounts[key.renderPass]++;
        return frameBuffer;
    }

    const VulkanRenderPass* VulkanFrameBufferCache::GetRenderPass(const RenderPassKey& key)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        auto iterator = m_renderPasses.find(key);

        if (iterator != m_renderPasses.end() && iterator->second.renderPass != nullptr)
        {
            iterator->second.pruneTick = nextPruneTick;
            return iterator->second.renderPass;
        }

        struct { VkImageLayout subpass, initial, final; } colorLayouts[PK_MAX_RENDER_TARGETS];
      
        // Is swap chain
        if (key.colors[0].layout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        {
            colorLayouts[0].initial = VK_IMAGE_LAYOUT_UNDEFINED;
            colorLayouts[0].final = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            colorLayouts[0].subpass = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        else 
        {
            for (int i = 0; i < PK_MAX_RENDER_TARGETS; ++i)
            {
                colorLayouts[i].initial = key.colors[i].layout;
                colorLayouts[i].final = key.colors[i].layout;
                colorLayouts[i].subpass = key.colors[i].layout;
            }
        }

        VkAttachmentReference colorAttachmentRefs[PK_MAX_RENDER_TARGETS] = {};
        VkAttachmentReference resolveAttachmentRef[PK_MAX_RENDER_TARGETS] = {};
        VkAttachmentReference depthAttachmentRef{};

        // Note that this needs to have the same ordering as the corollary array in getFramebuffer.
        VkAttachmentDescription attachments[PK_MAX_RENDER_TARGETS * 2 + 1] = {};

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
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

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

        int attachmentIndex = 0;

        for (int i = 0; i < PK_MAX_RENDER_TARGETS; i++) 
        {
            if (key.colors[i].format == VK_FORMAT_UNDEFINED) 
            {
                continue;
            }

            uint32_t index = subpass.colorAttachmentCount++;
            colorAttachmentRefs[index].layout = colorLayouts[i].subpass;
            colorAttachmentRefs[index].attachment = attachmentIndex;

            auto* attachment = attachments + attachmentIndex++;
            attachment->format = key.colors[i].format;
            attachment->samples = EnumConvert::GetSampleCountFlags(key.samples);
            attachment->loadOp = EnumConvert::GetLoadOp(key.colors[i].loadop);
            attachment->storeOp = EnumConvert::GetStoreOp(key.colors[i].storeop);
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment->initialLayout = colorLayouts[i].initial;
            attachment->finalLayout = colorLayouts[i].final;
        }

        VkAttachmentReference* pResolveAttachment = resolveAttachmentRef;
        for (int i = 0; i < PK_MAX_RENDER_TARGETS; ++i)
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
            attachment->finalLayout = colorLayouts[i].final;
        }

        if (hasDepth) 
        {
            depthAttachmentRef.layout = key.depth.layout;
            depthAttachmentRef.attachment = attachmentIndex;
            
            auto* attachment = attachments + attachmentIndex++;
            attachment->format = key.depth.format;
            attachment->samples = EnumConvert::GetSampleCountFlags(key.samples);
            attachment->loadOp = EnumConvert::GetLoadOp(key.depth.loadop);
            attachment->storeOp = EnumConvert::GetStoreOp(key.depth.storeop);
            attachment->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment->initialLayout = key.depth.layout;
            attachment->finalLayout = key.depth.layout;
            dependencies[0].srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies[0].dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependencies[0].dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }

        renderPassInfo.attachmentCount = attachmentIndex;

        auto renderPass = new VulkanRenderPass(m_device, renderPassInfo);
        m_renderPasses[key] = { renderPass, nextPruneTick };
        return renderPass;
    }

    void VulkanFrameBufferCache::Prune()
    {
        m_currentPruneTick++;

        for (auto& kv : m_framebuffers)
        {
            auto& value = kv.second;
            auto& key = kv.first;

            if (value.frameBuffer != nullptr && value.pruneTick < m_currentPruneTick)
            {
                m_renderPassReferenceCounts[key.renderPass]--;
                delete value.frameBuffer;
                value.frameBuffer = nullptr;
            }
        }

        for (auto& kv : m_renderPasses)
        {
            auto& value = kv.second;
            auto& key = kv.first;

            if (value.renderPass != nullptr && value.pruneTick < m_currentPruneTick && m_renderPassReferenceCounts[value.renderPass->renderPass] == 0)
            {
                delete value.renderPass;
                value.renderPass = nullptr;
            }
        }
    }
}