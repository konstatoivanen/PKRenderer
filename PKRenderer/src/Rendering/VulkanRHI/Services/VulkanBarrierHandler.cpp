#include "PrecompiledHeader.h"
#include "VulkanBarrierHandler.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Utilities/VectorUtilities.h"
#include "Math/FunctionsIntersect.h"
#include <vulkan/vk_enum_string_helper.h>

namespace PK::Rendering::VulkanRHI::Services
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::VulkanRHI::Utilities;
    using namespace PK::Math;

    struct urect1D
    {
        uint32_t xmin;
        uint32_t xmax;
    };

    struct urect2D
    {
        uint16_t xmin;
        uint16_t ymin;
        uint16_t xmax;
        uint16_t ymax;
    };

    uint32_t MaskedAdd(uint32_t x, uint32_t y)
    {
        auto t = (size_t)x + (size_t)y;
        return t >= 0xFFFFFFFFu ? 0xFFFFFFFFu : (uint32_t)t;
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsOverlap(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect1D*>(&a);
        auto rb = reinterpret_cast<urect1D*>(&b);
        return rb->xmin < (ra->xmin + ra->xmax) && (rb->xmin + rb->xmax) > ra->xmin;
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsAdjacent(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect1D*>(&a);
        auto rb = reinterpret_cast<urect1D*>(&b);
        return ra->xmin == (rb->xmin + rb->xmax) || rb->xmin == (ra->xmin + ra->xmax);
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsInclusive(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect1D*>(&a);
        auto rb = reinterpret_cast<urect1D*>(&b);
        return ra->xmin <= rb->xmin && (ra->xmin + ra->xmax) >= (rb->xmin + rb->xmax);
    }

    uint64_t VulkanBarrierHandler::TInfo<VkBuffer>::Merge(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect1D*>(&a);
        auto rb = reinterpret_cast<urect1D*>(&b);
        ra->xmax += ra->xmin;
        rb->xmax += rb->xmin;

        urect1D o;
        o.xmin = ra->xmin < rb->xmin ? ra->xmin : rb->xmin;
        o.xmax = (ra->xmax > rb->xmax ? ra->xmax : rb->xmax) - o.xmin;
        return *reinterpret_cast<uint64_t*>(&o);
    }

    uint32_t VulkanBarrierHandler::TInfo<VkBuffer>::Splice(uint64_t cv, uint64_t sv, uint64_t* ov)
    {
        auto n = 0u;
        auto c = reinterpret_cast<urect1D*>(&cv);
        auto s = reinterpret_cast<urect1D*>(&sv);
        auto o = reinterpret_cast<urect1D*>(ov);

        c->xmax += c->xmin;
        s->xmax += s->xmin;

        if (s->xmin > c->xmin && s->xmin < c->xmax)
        {
            o[n] = *c;
            o[n++].xmax = s->xmin;
        }

        if (s->xmax < c->xmax && s->xmax > c->xmin)
        {
            o[n] = *c;
            o[n++].xmin = s->xmax;
        }

        for (auto i = 0u; i < n; ++i)
        {
            o[i].xmax -= o[i].xmin;
        }

        return n;
    }


    void VulkanBarrierHandler::TInfo<VkImage>::SetDefaultRange(AccessRecord* a)
    {
        a->access = 0u;
        a->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        a->layout = VK_IMAGE_LAYOUT_UNDEFINED;
        a->imageRange.layer = 0u;
        a->imageRange.level = 0u;
        a->imageRange.layers = 0x7FFF;
        a->imageRange.levels = 0x7FFF;
    }

    bool VulkanBarrierHandler::TInfo<VkImage>::IsOverlap(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect2D*>(&a);
        auto rb = reinterpret_cast<urect2D*>(&b);
        return rb->xmin < (ra->xmin + ra->xmax) && (rb->xmin + rb->xmax) > ra->xmin &&
            rb->ymin < (ra->ymin + ra->ymax) && (rb->ymin + rb->ymax) > ra->ymin;
    }

    bool VulkanBarrierHandler::TInfo<VkImage>::IsAdjacent(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect2D*>(&a);
        auto rb = reinterpret_cast<urect2D*>(&b);
        return ((ra->xmin == (rb->xmin + rb->xmax) || rb->xmin == (ra->xmin + ra->xmax)) && (ra->ymin == rb->ymin && ra->ymax == rb->ymax)) ||
            ((ra->xmin == (rb->xmin + rb->xmax) || rb->xmin == (ra->xmin + ra->xmax)) && (ra->xmin == rb->xmin && ra->xmax == rb->xmax));
    }

    bool VulkanBarrierHandler::TInfo<VkImage>::IsInclusive(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect2D*>(&a);
        auto rb = reinterpret_cast<urect2D*>(&b);
        return ra->xmin <= rb->xmin && (ra->xmin + ra->xmax) >= (rb->xmin + rb->xmax) &&
            ra->ymin <= rb->ymin && (ra->ymin + ra->ymax) >= (rb->ymin + rb->ymax);
    }

    uint64_t VulkanBarrierHandler::TInfo<VkImage>::Merge(uint64_t a, uint64_t b)
    {
        auto ra = reinterpret_cast<urect2D*>(&a);
        auto rb = reinterpret_cast<urect2D*>(&b);
        ra->xmax += ra->xmin;
        ra->ymax += ra->ymin;
        rb->xmax += rb->xmin;
        rb->ymax += rb->ymin;

        urect2D o;
        o.xmin = ra->xmin < rb->xmin ? ra->xmin : rb->xmin;
        o.xmax = (ra->xmax > rb->xmax ? ra->xmax : rb->xmax) - o.xmin;
        o.ymin = ra->ymin < rb->ymin ? ra->ymin : rb->ymin;
        o.ymax = (ra->ymax > rb->ymax ? ra->ymax : rb->ymax) - o.ymin;
        return *reinterpret_cast<uint64_t*>(&o);
    }

    uint32_t VulkanBarrierHandler::TInfo<VkImage>::Splice(uint64_t cv, uint64_t sv, uint64_t* ov)
    {
        auto n = 0u;
        auto c = reinterpret_cast<urect2D*>(&cv);
        auto s = reinterpret_cast<urect2D*>(&sv);
        auto o = reinterpret_cast<urect2D*>(ov);

        c->xmax += c->xmin;
        c->ymax += c->ymin;
        s->xmax += s->xmin;
        s->ymax += s->ymin;

        if (s->xmin > c->xmin && s->xmin < c->xmax)
        {
            o[n] = *c;
            o[n++].xmax = s->xmin;
            c->xmin = s->xmin;
        }

        if (s->xmax < c->xmax && s->xmax > c->xmin)
        {
            o[n] = *c;
            o[n++].xmin = s->xmax;
            c->xmax = s->xmax;
        }

        if (s->ymin > c->ymin && s->ymin < c->ymax)
        {
            o[n] = *c;
            o[n++].ymax = s->ymin;
        }

        if (s->ymax < c->ymax && s->ymax > c->ymin)
        {
            o[n] = *c;
            o[n++].ymin = s->ymax;
        }

        for (auto i = 0u; i < n; ++i)
        {
            o[i].xmax -= o[i].xmin;
            o[i].ymax -= o[i].ymin;
        }

        return n;
    }


    void VulkanBarrierHandler::TransferRecords(VulkanBarrierHandler* target)
    {
        auto keyValues = m_resources.GetKeyValues();

        for (auto i = 0u; i < m_resources.GetCount(); ++i)
        {
            auto& key = keyValues.keys[i].key;
            auto index = target->m_resources.GetIndex(key);

            // Dont override newer data with older one
            if (index != -1 && target->m_accessTimestamps[index] >= m_accessTimestamps[i])
            {
                continue;
            }

            auto current = &keyValues.values[i];

            while (current && *current)
            {
                auto copy = **current;

                //@TODO FIX THIS Hack to ignore queue families for now
                copy.queueFamily = copy.queueFamily != 0xFFFF ? target->m_queueFamily : 0xFFFF;
                copy.access = 0u;
                copy.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

                // Hacky way to detect type. buffers will have zero value
                if ((*current)->aspect != 0u)
                {
                    // No need to transfer unitialized images as they should be handled by Record function default range anyways.
                    // Also this could lead to redundant barriers between queues.
                    if (copy.layout != VK_IMAGE_LAYOUT_UNDEFINED)
                    {
                        target->Record(reinterpret_cast<VkImage>(key), copy, PK_ACCESS_OPT_TRANSFER);
                    }
                }
                else
                {
                    target->Record(reinterpret_cast<VkBuffer>(key), copy, PK_ACCESS_OPT_TRANSFER);
                }

                current = &(*current)->next;
            }
        }
    }

    bool VulkanBarrierHandler::Resolve(VulkanBarrierInfo* outBarrierInfo)
    {
        if (outBarrierInfo == nullptr || (m_bufferBarriers.GetCount() == 0u && m_imageBarriers.GetCount() == 0u))
        {
            return false;
        }

        outBarrierInfo->memoryBarrierCount = 0u;
        outBarrierInfo->pMemoryBarriers = nullptr;
        outBarrierInfo->dependencyFlags = 0u;
        outBarrierInfo->srcStageMask = m_sourceStage;
        outBarrierInfo->dstStageMask = m_destinationStage;
        outBarrierInfo->pBufferMemoryBarriers = m_bufferBarriers.GetData();
        outBarrierInfo->bufferMemoryBarrierCount = (uint32_t)m_bufferBarriers.GetCount();
        outBarrierInfo->pImageMemoryBarriers = m_imageBarriers.GetData();
        outBarrierInfo->imageMemoryBarrierCount = (uint32_t)m_imageBarriers.GetCount();
        m_bufferBarriers.SetCount(0u);
        m_imageBarriers.SetCount(0u);
        m_sourceStage = 0u;
        m_destinationStage = 0u;
        return true;
    }

    void VulkanBarrierHandler::Prune()
    {
        for (auto i = (int32_t)(m_resources.GetCount() - 1); i >= 0; --i)
        {
            if (m_accessMask.GetAt(i))
            {
                continue;
            }

            auto record = m_resources.GetValueAt(i);
            m_resources.RemoveAt(i);

            while (record)
            {
                auto next = record->next;
                m_records.Delete(record);
                record = next;
            }
        }

        m_accessMask.Clear();
    }


    template<>
    void VulkanBarrierHandler::ProcessBarrier<VkBuffer, VkBufferMemoryBarrier>(const VkBuffer resource, VkBufferMemoryBarrier** barrier, const AccessRecord& recordOld, const AccessRecord& recordNew)
    {
        auto rangeo = *reinterpret_cast<const urect1D*>(&recordOld.range);
        auto rangen = *reinterpret_cast<const urect1D*>(&recordNew.range);
        rangeo.xmax += rangeo.xmin;
        rangen.xmax += rangen.xmin;

        // Expand queue family bits
        uint32_t srcQueueFamily = recordOld.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordOld.queueFamily;
        uint32_t dstQueueFamily = recordNew.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordNew.queueFamily;

        if (!(*barrier))
        {
            *barrier = m_bufferBarriers.Add();
            (*barrier)->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            (*barrier)->buffer = resource;
            (*barrier)->srcQueueFamilyIndex = srcQueueFamily;
            (*barrier)->dstQueueFamilyIndex = dstQueueFamily;
            (*barrier)->srcAccessMask = 0u;
            (*barrier)->dstAccessMask = recordNew.access;
            m_destinationStage |= recordNew.stage;
        }
        else
        {
            auto offset = (uint32_t)(*barrier)->offset;
            auto size = (uint32_t)(*barrier)->size;
            rangeo.xmin = rangeo.xmin < offset ? rangeo.xmin : offset;
            rangeo.xmax = rangeo.xmax > (offset + size) ? rangeo.xmax : (offset + size);
        }

        (*barrier)->offset = rangeo.xmin > rangen.xmin ? rangeo.xmin : rangen.xmin;
        (*barrier)->size = (rangeo.xmax < rangen.xmax ? rangeo.xmax : rangen.xmax) - (*barrier)->offset;

        m_sourceStage |= recordOld.stage;
        (*barrier)->srcAccessMask |= recordOld.access;
    }

    template<>
    void VulkanBarrierHandler::ProcessBarrier<VkImage, VkImageMemoryBarrier>(const VkImage resource, VkImageMemoryBarrier** barrier, const AccessRecord& recordOld, const AccessRecord& recordNew)
    {
        auto rangeo = *reinterpret_cast<const urect2D*>(&recordOld.range);
        auto rangen = *reinterpret_cast<const urect2D*>(&recordNew.range);
        rangeo.xmax += rangeo.xmin;
        rangeo.ymax += rangeo.ymin;
        rangen.xmax += rangen.xmin;
        rangen.ymax += rangen.ymin;

        // Expand queue family bits
        uint32_t srcQueueFamily = recordOld.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordOld.queueFamily;
        uint32_t dstQueueFamily = recordNew.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordNew.queueFamily;

        // Create new barrier on layout miss match
        if (!(*barrier) || (*barrier)->oldLayout != recordOld.layout)
        {
            *barrier = m_imageBarriers.Add();
            (*barrier)->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            (*barrier)->image = resource;
            (*barrier)->srcQueueFamilyIndex = srcQueueFamily;
            (*barrier)->dstQueueFamilyIndex = dstQueueFamily;
            (*barrier)->srcAccessMask = 0u;
            (*barrier)->dstAccessMask = recordNew.access;
            (*barrier)->oldLayout = recordOld.layout;
            (*barrier)->newLayout = recordNew.layout;
            (*barrier)->subresourceRange.aspectMask = recordNew.aspect;
            m_destinationStage |= recordNew.stage;
        }
        else
        {
            auto rb = reinterpret_cast<uint32_t*>(&(*barrier)->subresourceRange.baseMipLevel);
            rangeo.xmin = rangeo.xmin < rb[0] ? rangeo.xmin : rb[0];
            rangeo.xmax = rangeo.xmax > MaskedAdd(rb[0], rb[1]) ? rangeo.xmax : MaskedAdd(rb[0], rb[1]);
            rangeo.ymin = rangeo.ymin < rb[2] ? rangeo.ymin : rb[2];
            rangeo.ymax = rangeo.ymax > MaskedAdd(rb[2], rb[3]) ? rangeo.ymax : MaskedAdd(rb[2], rb[3]);
        }

        rangeo.xmin = rangeo.xmin > rangen.xmin ? rangeo.xmin : rangen.xmin;
        rangeo.xmax = rangeo.xmax < rangen.xmax ? rangeo.xmax : rangen.xmax;
        rangeo.ymin = rangeo.ymin > rangen.ymin ? rangeo.ymin : rangen.ymin;
        rangeo.ymax = rangeo.ymax < rangen.ymax ? rangeo.ymax : rangen.ymax;

        (*barrier)->subresourceRange.baseMipLevel = rangeo.xmin;
        (*barrier)->subresourceRange.levelCount = rangeo.xmax - rangeo.xmin;
        (*barrier)->subresourceRange.baseArrayLayer = rangeo.ymin;
        (*barrier)->subresourceRange.layerCount = rangeo.ymax - rangeo.ymin;

        if (rangeo.xmax >= 0x7FFF)
        {
            (*barrier)->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        }

        if (rangeo.ymax >= 0x7FFF)
        {
            (*barrier)->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        }

        m_sourceStage |= recordOld.stage;
        (*barrier)->srcAccessMask |= recordOld.access;
    }
}