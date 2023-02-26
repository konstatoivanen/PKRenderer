#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FixedList.h"
#include "Utilities/PointerMap.h"
#include "Core/Services/Log.h"

namespace PK::Rendering::VulkanRHI::Services
{
    constexpr static const uint8_t PK_ACCESS_OPT_BARRIER = 1 << 0;
    constexpr static const uint8_t PK_ACCESS_OPT_LAYOUT = 1 << 1;
    constexpr static const uint8_t PK_ACCESS_OPT_ISTRANSFER = 1 << 2;

    class VulkanBarrierHandler : public PK::Utilities::NoCopy
    {
        public:
            struct AccessRecord
            {
                union
                {
                    struct Range
                    {
                        uint32_t offset = 0u;
                        uint32_t size = 0u;
                    }
                    bufferRange;

                    Structs::TextureViewRange imageRange;
                };

                VkPipelineStageFlags stage = 0u;
                VkAccessFlags access = 0u;
                AccessRecord* next = nullptr;
                uint16_t queueFamily = 0u;

                // Image only values
                uint16_t aspect = 0u; //VkImageAspectFlags
                VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;

                AccessRecord() {};
            };

            inline static void ConditionalCopy(AccessRecord* out, const AccessRecord& value)
            {
                if (out)
                {
                    *out = value;
                }
            }
        
            template<typename T> struct TInfo {};
            
            template<> struct TInfo<VkBuffer>
            { 
                using BarrierType = VkBufferMemoryBarrier; 
                static void Merge(AccessRecord& a, const AccessRecord& b);
                inline static const VkBuffer GetResource(const VulkanBindHandle* handle) { return handle->buffer.buffer; }
                static AccessRecord GetRecord(const VulkanBindHandle* handle);
                static void Set(AccessRecord& a, const AccessRecord& b);
                static bool IsOverlap(const AccessRecord& a, const AccessRecord& b);
                static bool IsInclusiveRange(const AccessRecord& a, const AccessRecord& b);
                static bool IsInclusive(const AccessRecord& a, const AccessRecord& b);
                static bool IsAccessDiff(const AccessRecord& a, const AccessRecord& b);
            };

            template<> struct TInfo<VkImage>
            { 
                using BarrierType = VkImageMemoryBarrier; 
                static void Merge(AccessRecord& a, const AccessRecord& b);
                inline static const VkImage GetResource(const VulkanBindHandle* handle) { return handle->image.image; }
                static AccessRecord GetRecord(const VulkanBindHandle* handle);
                static void Set(AccessRecord& a, const AccessRecord& b);
                static bool IsOverlap(const AccessRecord& a, const AccessRecord& b);
                static bool IsInclusiveRange(const AccessRecord& a, const AccessRecord& b);
                static bool IsInclusive(const AccessRecord& a, const AccessRecord& b);
                static bool IsAccessDiff(const AccessRecord& a, const AccessRecord& b);
            };

            VulkanBarrierHandler(uint32_t queueFamily) : m_queueFamily(queueFamily) {};
            ~VulkanBarrierHandler();

            template<typename T>
            void Record(const T resource, AccessRecord record, uint8_t options, AccessRecord* out)
            {
                typedef TInfo<T>::BarrierType TBarrier;
                TBarrier* barrier = nullptr;

                auto key = reinterpret_cast<uint64_t>(resource);
                auto index = m_resources.GetIndex(key);

                // First value no resolve
                if (index == -1)
                {
                    auto newRecord = m_records.New(record);
                    m_resources.AddValue(key, newRecord);
                    m_pruneTicks.push_back(m_currentPruneTick + 1ull);
                    ConditionalCopy(out, record);

                    if (options & PK_ACCESS_OPT_LAYOUT)
                    {
                        record.access = 0u;
                        record.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                        record.layout = VK_IMAGE_LAYOUT_UNDEFINED;
                        ProcessBarrier<T>(resource, &barrier, record, *newRecord, true);
                    }

                    return;
                }
                
                AccessRecord** current = m_resources.GetValueAtRef(index);
                m_pruneTicks.at(index) = m_currentPruneTick + 1ull;

                while (current && *current)
                {
                    if (TInfo<T>::IsInclusive(**current, record))
                    {
                        return;
                    }

                    auto overlap = TInfo<T>::IsOverlap(**current, record);
                    auto accessDiff = TInfo<T>::IsAccessDiff(**current, record);

                    // Resolve
                    if (overlap && accessDiff)
                    {
                        if (options & PK_ACCESS_OPT_BARRIER)
                        {
                            ProcessBarrier<T>(resource, &barrier, **current, record, false);
                        }
                       
                        if ((*current)->next == nullptr)
                        {
                            ConditionalCopy(out, **current);
                            TInfo<T>::Set(**current, record);
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
                            ConditionalCopy(out, **current);
                            TInfo<T>::Merge(**current, record);
                            return;
                        }

                        TInfo<T>::Merge(record, **current);
                        *current = RemoveRecord(*current);
                        continue;
                    }

                    // Append
                    if ((*current)->next == nullptr)
                    {
                        (*current)->next = m_records.New(record);
                        ConditionalCopy(out, record);
                        return;
                    }

                    current = &(*current)->next;
                }
            }

            template<typename T>
            void Record(const VulkanBindHandle* handle, 
                        VkAccessFlags access, 
                        VkPipelineStageFlags stage,
                        VkImageLayout overrideLayout = VK_IMAGE_LAYOUT_MAX_ENUM,
                        uint8_t options = PK_ACCESS_OPT_BARRIER,
                        AccessRecord* out = nullptr)
            {
                auto resource = TInfo<T>::GetResource(handle);
                auto record = TInfo<T>::GetRecord(handle);
                record.queueFamily = handle->isConcurrent ? VK_QUEUE_FAMILY_IGNORED : m_queueFamily;
                record.access = access;
                record.stage = stage;

                if (overrideLayout != VK_IMAGE_LAYOUT_MAX_ENUM)
                {
                    record.layout = overrideLayout;
                }

                Record(resource, record, options, out);
            }

            void TransferRecords(VulkanBarrierHandler* target);
            bool Resolve(VulkanBarrierInfo* outBarrierInfo, bool isQueueTransfer);
            void Prune();

        private:
            // @TODO do proper transfers
            template<typename T>
            void Transfer(const uint64_t key, AccessRecord record)
            {
                auto index = m_resources.GetIndex(key);
                record.next = nullptr;
                record.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                record.access = VK_ACCESS_NONE;
                record.queueFamily = record.queueFamily != 0xFFFF ? m_queueFamily : 0xFFFF;

                // First value no resolve
                if (index == -1)
                {
                    auto newRecord = m_records.New(record);
                    m_resources.AddValue(key, newRecord);
                    m_pruneTicks.push_back(0ull);
                    return;
                }
                
                AccessRecord** current = m_resources.GetValueAtRef(index);

                while (current && *current)
                {
                    if (TInfo<T>::IsInclusive(**current, record))
                    {
                        return;
                    }

                    auto overlap = TInfo<T>::IsOverlap(**current, record);
                    auto accessDiff = TInfo<T>::IsAccessDiff(**current, record);

                    // Resolve
                    if (overlap && accessDiff)
                    {
                        if ((*current)->next == nullptr)
                        {
                            TInfo<T>::Set(**current, record);
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
                            TInfo<T>::Merge(**current, record);
                            return;
                        }

                        TInfo<T>::Merge(record, **current);
                        *current = RemoveRecord(*current);
                        continue;
                    }

                    // Append
                    if ((*current)->next == nullptr)
                    {
                        (*current)->next = m_records.New(record);
                        return;
                    }

                    current = &(*current)->next;
                }
            }

            template<typename T, typename TBarrier>
            void ProcessBarrier(const T resource, 
                                TBarrier** barrier, 
                                const AccessRecord& recordOld, 
                                const AccessRecord& recordNew, 
                                bool useFullRange);
            AccessRecord* RemoveRecord(AccessRecord* record);

            const uint32_t m_queueFamily = 0u;
            PK::Utilities::PointerMap<uint64_t, AccessRecord> m_resources;
            PK::Utilities::FixedPool<AccessRecord, 1024> m_records;
            PK::Utilities::FixedList<VkBufferMemoryBarrier, 256> m_bufferBarriers;
            PK::Utilities::FixedList<VkImageMemoryBarrier, 256> m_imageBarriers;
            std::vector<uint64_t> m_pruneTicks;
            VkPipelineStageFlags m_sourceStage = 0u;
            VkPipelineStageFlags m_destinationStage = 0u;
            uint32_t m_srcQueueFamily = VK_QUEUE_FAMILY_IGNORED;
            uint32_t m_dstQueueFamily = VK_QUEUE_FAMILY_IGNORED;
            uint64_t m_currentPruneTick = 0u;
            uint64_t m_transferCount = 0u;
    };
}