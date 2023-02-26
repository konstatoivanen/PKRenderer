#include "PrecompiledHeader.h"
#include "VulkanBarrierHandler.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Utilities/VectorUtilities.h"
#include <vulkan/vk_enum_string_helper.h>

namespace PK::Rendering::VulkanRHI::Services
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::VulkanRHI::Utilities;

    template<typename T0, typename T1>
    inline static void IncludeRange(T0* offset, T0* size, T1 newOffset, T1 newSize)
    {
        auto min0 = *offset;
        auto max0 = *offset + *size;
        auto min1 = newOffset;
        auto max1 = newOffset + newSize;
        *offset = min0 < min1 ? min0 : min1;
        *size = (max0 > max1 ? max0 : max1) - *offset;
    }

    inline static void IncludeRange(VkImageSubresourceRange* range0, const Structs::TextureViewRange& range1)
    {
        IncludeRange(&range0->baseArrayLayer, &range0->layerCount, (uint32_t)range1.layer, (uint32_t)range1.layers);
        IncludeRange(&range0->baseMipLevel, &range0->levelCount, (uint32_t)range1.level, (uint32_t)range1.levels);
    }

    inline static void IncludeRange(Structs::TextureViewRange* range0, const Structs::TextureViewRange& range1)
    {
        IncludeRange(&range0->layer, &range0->layers, range1.layer, range1.layers);
        IncludeRange(&range0->level, &range0->levels, range1.level, range1.levels);
    }

    VulkanBarrierHandler::AccessRecord VulkanBarrierHandler::TInfo<VkBuffer>::GetRecord(const VulkanBindHandle* handle)
    {
        AccessRecord record{};
        record.bufferRange.offset = (uint32_t)handle->buffer.offset;
        record.bufferRange.size = (uint32_t)handle->buffer.range;
        return record;
    }

    void VulkanBarrierHandler::TInfo<VkBuffer>::Merge(AccessRecord& a, const AccessRecord& b)
    {
        a.access |= b.access;
        a.stage |= b.stage;
        IncludeRange(&a.bufferRange.offset, &a.bufferRange.size, b.bufferRange.offset, b.bufferRange.size);
    }

    void VulkanBarrierHandler::TInfo<VkBuffer>::Set(AccessRecord& a, const AccessRecord& b)
    {
        a.access = b.access;
        a.stage = b.stage;
        a.bufferRange = b.bufferRange;
        a.queueFamily = b.queueFamily;
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsOverlap(const AccessRecord& a, const AccessRecord& b)
    {
        return b.bufferRange.offset < (a.bufferRange.offset + a.bufferRange.size) &&
            (b.bufferRange.offset + b.bufferRange.size) > a.bufferRange.offset;
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsInclusiveRange(const AccessRecord& a, const AccessRecord& b)
    {
        return a.bufferRange.offset <= b.bufferRange.offset &&
            (a.bufferRange.offset + a.bufferRange.size) >= (b.bufferRange.offset + b.bufferRange.size);
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsInclusive(const AccessRecord& a, const AccessRecord& b)
    {
        return IsInclusiveRange(a, b) && a.stage == b.stage && a.access == b.access && a.queueFamily == b.queueFamily;
    }

    bool VulkanBarrierHandler::TInfo<VkBuffer>::IsAccessDiff(const AccessRecord& a, const AccessRecord& b)
    {
        auto read0 = EnumConvert::IsReadAccess(b.access);
        auto write0 = EnumConvert::IsWriteAccess(b.access);
        auto read1 = EnumConvert::IsReadAccess(a.access);
        auto write1 = EnumConvert::IsWriteAccess(a.access);
        return (write1 && (read0 || write0)) || (write0 && (read1 || write1)) || a.queueFamily != b.queueFamily;
    }

    
    VulkanBarrierHandler::AccessRecord VulkanBarrierHandler::TInfo<VkImage>::GetRecord(const VulkanBindHandle* handle)
    {
        AccessRecord record{};
        record.imageRange = Utilities::VulkanConvertRange(handle->image.range);
        record.layout = handle->image.layout;
        record.aspect = handle->image.range.aspectMask;
        return record;
    }

    void VulkanBarrierHandler::TInfo<VkImage>::Merge(AccessRecord& a, const AccessRecord& b)
    {
        assert(a.layout == b.layout);
        a.access |= b.access;
        a.stage |= b.stage;
        IncludeRange(&a.imageRange, b.imageRange);
    }

    void VulkanBarrierHandler::TInfo<VkImage>::Set(AccessRecord& a, const AccessRecord& b)
    {
        a.access = b.access;
        a.stage = b.stage;
        a.layout = b.layout;
        a.imageRange = b.imageRange;
        a.queueFamily = b.queueFamily;
    }
    
    bool VulkanBarrierHandler::TInfo<VkImage>::IsOverlap(const AccessRecord& a, const AccessRecord& b)
    {
        return b.imageRange.layer < (a.imageRange.layer + a.imageRange.layers) &&
            (b.imageRange.layer + b.imageRange.layers) > a.imageRange.layer &&
            b.imageRange.level < (a.imageRange.level + b.imageRange.levels) &&
            (b.imageRange.level + b.imageRange.levels) > a.imageRange.level;
    }
    
    bool VulkanBarrierHandler::TInfo<VkImage>::IsInclusiveRange(const AccessRecord& a, const AccessRecord& b)
    {
        return a.imageRange.layer <= b.imageRange.layer &&
            (a.imageRange.layer + a.imageRange.layers) >= (b.imageRange.layer + b.imageRange.layers) &&
            a.imageRange.level <= b.imageRange.level &&
            (a.imageRange.level + a.imageRange.levels) >= (b.imageRange.level + b.imageRange.levels);
    }
    
    bool VulkanBarrierHandler::TInfo<VkImage>::IsInclusive(const AccessRecord& a, const AccessRecord& b)
    {
        return IsInclusiveRange(a, b) && a.stage == b.stage && a.access == b.access && a.layout == b.layout && a.queueFamily == b.queueFamily;
    }

    bool VulkanBarrierHandler::TInfo<VkImage>::IsAccessDiff(const AccessRecord& a, const AccessRecord& b)
    {
        auto read0 = EnumConvert::IsReadAccess(b.access);
        auto write0 = EnumConvert::IsWriteAccess(b.access);
        auto read1 = EnumConvert::IsReadAccess(a.access);
        auto write1 = EnumConvert::IsWriteAccess(a.access);
        return (write1 && (read0 || write0)) || (write0 && (read1 || write1)) || a.layout != b.layout || a.queueFamily != b.queueFamily;
    }


    VulkanBarrierHandler::~VulkanBarrierHandler()
    {
    }

    void VulkanBarrierHandler::TransferRecords(VulkanBarrierHandler* target)
    {
        auto keyValues = m_resources.GetKeyValues();

        for (auto i = 0u; i < m_resources.GetCount(); ++i)
        {
            // Dont copy unaccessed resources
            if (m_pruneTicks.at(i) < m_currentPruneTick)
            {
                continue;
            }

            auto& key = keyValues.keys[i].key;
            auto current = &keyValues.values[i];

            // Hacky way to detect type. buffers will have zero value
            if ((*current)->aspect != 0u)
            {
                while (current && *current)
                {
                    target->Transfer<VkImage>(key, **current);
                    current = &(*current)->next;
                }
            }
            else
            {
                while (current && *current)
                {
                    target->Transfer<VkBuffer>(key, **current);
                    current = &(*current)->next;
                }
            }
        }

        m_transferCount++;
        m_currentPruneTick++;
    }

    bool VulkanBarrierHandler::Resolve(VulkanBarrierInfo* outBarrierInfo, bool isQueueTransfer)
    {
        if (outBarrierInfo == nullptr || (m_bufferBarriers.GetCount() == 0u && m_imageBarriers.GetCount() == 0u))
        {
            return false;
        }

        if (isQueueTransfer && m_srcQueueFamily == m_dstQueueFamily)
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
        auto pruneTick = m_currentPruneTick - m_transferCount;

        for (auto i = (int32_t)(m_resources.GetCount() - 1); i >= 0; --i)
        {
            if (m_pruneTicks.at(i) > pruneTick)
            {
                continue;
            }

            auto record = m_resources.GetValueAt(i);
            PK::Utilities::Vector::UnorderedRemoveAt(m_pruneTicks, i);
            m_resources.RemoveAt(i);

            while (record)
            {
                auto next = record->next;
                m_records.Delete(record);
                record = next;
            }
        }

        m_transferCount = 0u;
        m_currentPruneTick++;
    }

    template<>
    void VulkanBarrierHandler::ProcessBarrier<VkBuffer, VkBufferMemoryBarrier>(
        const VkBuffer resource, 
        VkBufferMemoryBarrier** barrier, 
        const AccessRecord& recordOld, 
        const AccessRecord& recordNew,
        bool useFullRange)
    {
        // Expand queue family bits
        uint32_t srcQueueFamily = recordOld.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordOld.queueFamily;
        uint32_t dstQueueFamily = recordNew.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordNew.queueFamily;

        if (!(*barrier))
        {
            *barrier = m_bufferBarriers.Add();
            (*barrier)->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            (*barrier)->buffer = resource;
            (*barrier)->srcQueueFamilyIndex = srcQueueFamily;
            (*barrier)->dstQueueFamilyIndex = dstQueueFamily;// VK_QUEUE_FAMILY_IGNORED;
            (*barrier)->srcAccessMask = 0u;
            (*barrier)->dstAccessMask = recordNew.access;
            (*barrier)->offset = recordNew.bufferRange.offset;
            (*barrier)->size = recordNew.bufferRange.size;
            m_destinationStage |= recordNew.stage;
            m_srcQueueFamily = srcQueueFamily;
            m_dstQueueFamily = dstQueueFamily;

            if (useFullRange)
            {
                (*barrier)->offset = 0ull;
                (*barrier)->size = VK_WHOLE_SIZE;
            }
        }

        PK_THROW_ASSERT(m_srcQueueFamily == srcQueueFamily, "Cannot batch multiple queue family barriers together!");

        m_sourceStage |= recordOld.stage;
        (*barrier)->srcAccessMask |= recordOld.access;
        IncludeRange(&(*barrier)->offset, &(*barrier)->size, recordOld.bufferRange.offset, recordOld.bufferRange.size);
    }

    template<>
    void VulkanBarrierHandler::ProcessBarrier<VkImage, VkImageMemoryBarrier>(
        const VkImage resource, 
        VkImageMemoryBarrier** barrier, 
        const AccessRecord& recordOld, 
        const AccessRecord& recordNew,
        bool useFullRange)
    {
        // Expand queue family bits
        uint32_t srcQueueFamily = recordOld.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordOld.queueFamily;
        uint32_t dstQueueFamily = recordNew.queueFamily == 0xFFFF ? VK_QUEUE_FAMILY_IGNORED : recordNew.queueFamily;

        // Create new barrier on layout miss match
        if (!(*barrier) || (*barrier)->oldLayout != recordOld.layout)
        {
            *barrier = m_imageBarriers.Add();
            (*barrier)->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            (*barrier)->image = resource;
            (*barrier)->srcQueueFamilyIndex = srcQueueFamily;// VK_QUEUE_FAMILY_IGNORED;
            (*barrier)->dstQueueFamilyIndex = dstQueueFamily;// VK_QUEUE_FAMILY_IGNORED;
            (*barrier)->srcAccessMask = 0u;
            (*barrier)->dstAccessMask = recordNew.access;
            (*barrier)->oldLayout = recordOld.layout;
            (*barrier)->newLayout = recordNew.layout;
            (*barrier)->subresourceRange = Utilities::VulkanConvertRange(recordNew.imageRange, recordNew.aspect);
            m_destinationStage |= recordNew.stage;
            m_srcQueueFamily = srcQueueFamily;
            m_dstQueueFamily = dstQueueFamily;

            if (useFullRange)
            {
                (*barrier)->subresourceRange.baseArrayLayer = 0u;
                (*barrier)->subresourceRange.baseMipLevel = 0u;
                (*barrier)->subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
                (*barrier)->subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
            }
        }

        PK_THROW_ASSERT(m_srcQueueFamily == srcQueueFamily, "Cannot batch multiple queue family barriers together!");

        m_sourceStage |= recordOld.stage;
        (*barrier)->srcAccessMask |= recordOld.access;
        IncludeRange(&(*barrier)->subresourceRange, recordOld.imageRange);
    }

    VulkanBarrierHandler::AccessRecord* VulkanBarrierHandler::RemoveRecord(AccessRecord* record)
    {
        auto nextPointer = record->next;
        m_records.Delete(record);
        return nextPointer;
    }
}