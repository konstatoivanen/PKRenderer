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

    template<typename T>
    inline static void ConditionalCopy(T* dst, const T& src)
    {
        if (dst)
        {
            *dst = src;
        }
    }

    template<>
    void VulkanBarrierHandler::AccessRecord::Set<VkBuffer>(const AccessRecord& record)
    {
        access = record.access;
        stage = record.stage;
        bufferRange = record.bufferRange;
    }

    template<>
    void VulkanBarrierHandler::AccessRecord::Merge<VkBuffer>(const AccessRecord& record)
    {
        access |= record.access;
        stage |= record.stage;
        IncludeRange(&bufferRange.offset, &bufferRange.size, record.bufferRange.offset, record.bufferRange.size);
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsOverlap<VkBuffer>(const AccessRecord& record) const
    {
        const auto min0 = bufferRange.offset;
        const auto max0 = bufferRange.offset + bufferRange.size;
        const auto min1 = record.bufferRange.offset;
        const auto max1 = record.bufferRange.offset + record.bufferRange.size;
        return min1 < max0 && max1 > min0;
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsAdjacent<VkBuffer>(const AccessRecord& record) const
    {
        const auto min0 = bufferRange.offset;
        const auto max0 = bufferRange.offset + bufferRange.size;
        const auto min1 = record.bufferRange.offset;
        const auto max1 = record.bufferRange.offset + record.bufferRange.size;
        return min0 == max1 || min1 == max0;
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsInclusive<VkBuffer>(const AccessRecord& record) const
    {
        const auto min0 = bufferRange.offset;
        const auto max0 = bufferRange.offset + bufferRange.size;
        const auto min1 = record.bufferRange.offset;
        const auto max1 = record.bufferRange.offset + record.bufferRange.size;
        return min0 <= min1 && max0 >= max1 && stage == record.stage && access == record.access;
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsAccessDiff<VkBuffer>(const AccessRecord& record) const
    {
        auto read0 = EnumConvert::IsReadAccess(record.access);
        auto write0 = EnumConvert::IsWriteAccess(record.access);
        auto read1 = EnumConvert::IsReadAccess(access);
        auto write1 = EnumConvert::IsWriteAccess(access);

        return (write1 && (read0 || write0)) || 
               (write0 && (read1 || write1));
    }

    template<>
    void VulkanBarrierHandler::AccessRecord::Set<VkImage>(const AccessRecord& record)
    {
        access = record.access;
        stage = record.stage;
        layout = record.layout;
        imageRange = record.imageRange;
    }

    template<>
    void VulkanBarrierHandler::AccessRecord::Merge<VkImage>(const AccessRecord& record)
    {
        assert(layout == record.layout);
        access |= record.access;
        stage |= record.stage;
        IncludeRange(&imageRange, record.imageRange);
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsOverlap<VkImage>(const AccessRecord& record) const
    {
        const auto min00 = imageRange.layer;
        const auto max00 = imageRange.layer + imageRange.layers;
        const auto min01 = record.imageRange.layer;
        const auto max01 = record.imageRange.layer + record.imageRange.layers;

        const auto min10 = imageRange.level;
        const auto max10 = imageRange.level + imageRange.levels;
        const auto min11 = record.imageRange.level;
        const auto max11 = record.imageRange.level + record.imageRange.levels;

        return min01 < max00 && max01 > min00 && 
               min11 < max10 && max11 > min10;
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsInclusive<VkImage>(const AccessRecord& record) const
    {
        const auto min00 = imageRange.layer;
        const auto max00 = imageRange.layer + imageRange.layers;
        const auto min01 = record.imageRange.layer;
        const auto max01 = record.imageRange.layer + record.imageRange.layers;

        const auto min10 = imageRange.level;
        const auto max10 = imageRange.level + imageRange.levels;
        const auto min11 = record.imageRange.level;
        const auto max11 = record.imageRange.level + record.imageRange.levels;

        return min00 <= min01 && max00 >= max01 && 
               min10 <= min11 && max10 >= max11 && 
               stage == record.stage && access == record.access && layout == record.layout;
    }

    template<>
    bool VulkanBarrierHandler::AccessRecord::IsAccessDiff<VkImage>(const AccessRecord& record) const
    {
        auto read0 = EnumConvert::IsReadAccess(record.access);
        auto write0 = EnumConvert::IsWriteAccess(record.access);
        auto read1 = EnumConvert::IsReadAccess(access);
        auto write1 = EnumConvert::IsWriteAccess(access);

        return (write1 && (read0 || write0)) || 
               (write0 && (read1 || write1)) ||
               layout != record.layout;
    }


    VulkanBarrierHandler::~VulkanBarrierHandler()
    {
    }

    template<>
    void VulkanBarrierHandler::Record<VkBuffer>(const VkBuffer buffer, AccessRecord record, AccessRecord* outValue, bool writeBarrier)
    {
        auto key = reinterpret_cast<uint64_t>(buffer);
        auto index = m_resources.GetIndex(key);

        // First value no resolve
        if (index == -1)
        {
            m_resources.AddValue(key, m_records.New(buffer, record));
            m_pruneTicks.push_back(m_currentPruneTick + 1ull);
            ConditionalCopy(outValue, record);
            return;
        }

        VkBufferMemoryBarrier* barrier = nullptr;
        AccessRecord** current = m_resources.GetValueAtRef(index);
        m_pruneTicks.at(index) = m_currentPruneTick + 1ull;

        while (current && *current)
        {
            if ((*current)->IsInclusive<VkBuffer>(record))
            {
                return;
            }

            auto overlap = (*current)->IsOverlap<VkBuffer>(record);
            auto adjacent = (*current)->IsAdjacent<VkBuffer>(record);
            auto accessDiff = (*current)->IsAccessDiff<VkBuffer>(record);

            // Resolve
            if (overlap && accessDiff)
            {
                if (writeBarrier)
                {
                    if (!barrier)
                    {
                        barrier = m_bufferBarriers.Add();
                        barrier->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
                        barrier->buffer = buffer;
                        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier->srcAccessMask = 0u;
                        barrier->dstAccessMask = record.access;
                        barrier->offset = record.bufferRange.offset;
                        barrier->size = record.bufferRange.size;
                        m_destinationStage |= record.stage;
                    }

                    m_sourceStage |= (*current)->stage;
                    barrier->srcAccessMask |= (*current)->access;
                    IncludeRange(&barrier->offset, &barrier->size, (*current)->bufferRange.offset, (*current)->bufferRange.size);
                }
               
                if ((*current)->next == nullptr)
                {
                    ConditionalCopy(outValue, **current);
                    (*current)->Set<VkBuffer>(record);
                    return;
                }

                *current = RemoveRecord(*current);
                continue;
            }

            // Merge
            if ((overlap || adjacent) && !accessDiff)
            {
                if ((*current)->next == nullptr)
                {
                    (*current)->Merge<VkBuffer>(record);
                    ConditionalCopy(outValue, **current);
                    return;
                }

                record.Merge<VkBuffer>(**current);
                *current = RemoveRecord(*current);
                continue;
            }

            // Append
            if ((*current)->next == nullptr)
            {
                (*current)->next = m_records.New(buffer, record);
                ConditionalCopy(outValue, *(*current)->next);
                return;
            }

            current = &(*current)->next;
        }
    }
    
    template<>
    void VulkanBarrierHandler::Record<VkImage>(const VkImage image, AccessRecord record, AccessRecord* outValue, bool writeBarrier)
    {
        auto key = reinterpret_cast<uint64_t>(image);
        auto index = m_resources.GetIndex(key);

        // First value no resolve
        if (index == -1)
        {
            m_resources.AddValue(key, m_records.New(image, record)); 
            m_pruneTicks.push_back(m_currentPruneTick + 1ull);
            ConditionalCopy(outValue, record);
            return;
        }

        VkImageMemoryBarrier* barrier = nullptr;
        AccessRecord** current = m_resources.GetValueAtRef(index);
        m_pruneTicks.at(index) = m_currentPruneTick + 1ull;

        while (current && *current)
        {
            if ((*current)->IsInclusive<VkImage>(record))
            {
                ConditionalCopy(outValue, record);
                return;
            }

            auto overlap = (*current)->IsOverlap<VkImage>(record);
            auto accessDiff = (*current)->IsAccessDiff<VkImage>(record);

            // Resolve
            if (overlap && accessDiff)
            {
                if (writeBarrier)
                {
                    // Create new barrier on layout miss match
                    if (!barrier || barrier->oldLayout != (*current)->layout)
                    {
                        barrier = m_imageBarriers.Add();
                        barrier->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
                        barrier->image = image;
                        barrier->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
                        barrier->srcAccessMask = 0u;
                        barrier->dstAccessMask = record.access;
                        barrier->oldLayout = (*current)->layout;
                        barrier->newLayout = record.layout;
                        barrier->subresourceRange = Utilities::VulkanConvertRange(record.imageRange, record.aspect);
                        m_destinationStage |= record.stage;
                    }

                    m_sourceStage |= (*current)->stage;
                    barrier->srcAccessMask |= (*current)->access;
                    IncludeRange(&barrier->subresourceRange, (*current)->imageRange);
                }
               
                if ((*current)->next == nullptr)
                {
                    ConditionalCopy(outValue, **current);
                    (*current)->Set<VkImage>(record);
                    return;
                }

                *current = RemoveRecord(*current);
                continue;
            }

            // Merge
            if (overlap && !accessDiff)
            {
                if ((*current)->next == nullptr)
                {
                    (*current)->Merge<VkImage>(record);
                    ConditionalCopy(outValue, **current);
                    return;
                }

                record.Merge<VkImage>(**current);
                *current = RemoveRecord(*current);
                continue;
            }

            // Append
            if ((*current)->next == nullptr)
            {
                (*current)->next = m_records.New(image, record);
                ConditionalCopy(outValue, record);
                return;
            }

            current = &(*current)->next;
        }
    }


    bool VulkanBarrierHandler::Resolve(VulkanBarrierInfo* outBarrierInfo)
    {
        if (m_bufferBarriers.GetCount() == 0u && m_imageBarriers.GetCount() == 0u)
        {
            return false;
        }

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
            if (m_pruneTicks.at(i) > m_currentPruneTick)
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

        ++m_currentPruneTick;
    }

    VulkanBarrierHandler::AccessRecord* VulkanBarrierHandler::RemoveRecord(AccessRecord* record)
    {
        auto nextPointer = record->next;
        m_records.Delete(record);
        return nextPointer;
    }
}