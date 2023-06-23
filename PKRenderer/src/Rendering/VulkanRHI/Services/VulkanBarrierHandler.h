#pragma once
#include "Utilities/NoCopy.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FixedList.h"
#include "Utilities/FastMap.h"

namespace PK::Rendering::VulkanRHI::Services
{
    constexpr static const uint8_t PK_ACCESS_OPT_BARRIER = 1 << 0;
    constexpr static const uint8_t PK_ACCESS_OPT_TRANSFER = 1 << 1;

    class VulkanBarrierHandler : public PK::Utilities::NoCopy
    {
        public:
            struct AccessRecord
            {
                union
                {
                    uint64_t range = 0u;
                    Structs::TextureViewRange imageRange;

                    struct Range
                    {
                        uint32_t offset;
                        uint32_t size;
                    }
                    bufferRange;
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

            template<typename T> struct TInfo {};
            
            template<> struct TInfo<VkBuffer>
            { 
                using BarrierType = VkBufferMemoryBarrier; 
                static void SetDefaultRange(AccessRecord* a) {};
                static bool IsOverlap(uint64_t a, uint64_t b);
                static bool IsAdjacent(uint64_t a, uint64_t b);
                static bool IsInclusive(uint64_t  a, uint64_t b);
                static uint64_t Merge(uint64_t a, uint64_t b);
                static uint32_t Splice(uint64_t c, uint64_t s, uint64_t* o);
            };

            template<> struct TInfo<VkImage>
            { 
                using BarrierType = VkImageMemoryBarrier; 
                static void SetDefaultRange(AccessRecord* a);
                static bool IsOverlap(uint64_t a, uint64_t b);
                static bool IsAdjacent(uint64_t a, uint64_t b);
                static bool IsInclusive(uint64_t a, uint64_t b);
                static uint64_t Merge(uint64_t a, uint64_t b);
                static uint32_t Splice(uint64_t c, uint64_t s, uint64_t* o);
            };

            VulkanBarrierHandler(uint32_t queueFamily) : m_queueFamily(queueFamily) {};

            constexpr uint32_t GetQueueFamily() const { return m_queueFamily; }

            template<typename T>
            void Record(const T resource, const AccessRecord& record, uint8_t options)
            {
                typedef TInfo<T>::BarrierType TBarrier;
                TBarrier* barrier = nullptr;

                auto scope = record;
                scope.next = nullptr;
                
                auto key = reinterpret_cast<uint64_t>(resource);
                auto index = m_resources.GetIndex(key);

                if (index == -1)
                {
                    auto defaultRecord = m_records.New(scope);
                    TInfo<T>::SetDefaultRange(defaultRecord);
                    m_resources.AddValue(key, defaultRecord);
                    index = m_resources.GetCount() - 1;
                }

                m_transferMask.SetAt(index, (options & PK_ACCESS_OPT_TRANSFER) == 0u);

                if ((options & PK_ACCESS_OPT_TRANSFER) == 0u)
                {
                    m_accessMask.SetAt(index, true);
                }

                auto current = m_resources.GetValueAtRef(index);

                for (auto next = &(*current)->next; *current; current = next, next = &(*current)->next)
                {
                    if (TInfo<T>::IsInclusive((*current)->range, scope.range) &&
                        (*current)->stage == scope.stage &&
                        (*current)->access == scope.access &&
                        (*current)->layout == scope.layout &&
                        (*current)->queueFamily == scope.queueFamily)
                    {
                        return;
                    }

                    auto r0 = EnumConvert::IsReadAccess((*current)->access);
                    auto w0 = EnumConvert::IsWriteAccess((*current)->access);
                    auto r1 = EnumConvert::IsReadAccess(scope.access);
                    auto w1 = EnumConvert::IsWriteAccess(scope.access);

                    auto overlap = TInfo<T>::IsOverlap((*current)->range, scope.range);
                    auto adjacent = TInfo<T>::IsAdjacent((*current)->range, scope.range);
                    auto mergeFlags = (*current)->layout == scope.layout && (*current)->queueFamily == scope.queueFamily;
                    auto mergeOverlap = overlap && (!w1 || (!r0 && !w0)) && (!w0 || (!r1 && !w1));
                    auto mergeAdjacent = adjacent && w0 == w1 && r0 == r1;

                    if (mergeFlags && (mergeOverlap || mergeAdjacent))
                    {
                        scope.stage |= (*current)->stage;
                        scope.access |= (*current)->access;
                        scope.range = TInfo<T>::Merge((*current)->range, scope.range);
                        Delete(current, &next);
                        continue;
                    }

                    if (!overlap)
                    {
                        continue;
                    }
        
                    if (options & PK_ACCESS_OPT_BARRIER)
                    {
                        ProcessBarrier<T>(resource, &barrier, **current, record);
                    }

                    // Same or inclusive current range
                    if (TInfo<T>::IsInclusive(scope.range, (*current)->range))
                    {
                        Delete(current, &next);
                        continue;
                    }

                    uint64_t ranges[4];
                    auto count = TInfo<T>::Splice((*current)->range, scope.range, ranges);

                    (*current)->range = ranges[0];

                    for (auto i = 1u; i < count; ++i)
                    {
                        auto slice = m_records.New(**current);
                        slice->range= ranges[i];
                        slice->next = (*current)->next;
                        next = &slice->next;

                        (*current)->next = slice;
                        current = &(*current)->next;
                    }
                }

                *current = m_records.New(scope);
            }

            template<typename T>
            AccessRecord Retrieve(const T resource, const AccessRecord& record) const
            {
                auto index = m_resources.GetIndex(reinterpret_cast<uint64_t>(resource));
                auto previous = record;

                previous.access = VK_ACCESS_NONE;
                previous.stage = VK_PIPELINE_STAGE_NONE;
                previous.layout = VK_IMAGE_LAYOUT_MAX_ENUM;
                
                if (index != -1)
                {
                    for (auto cur = m_resources.GetValueAt(index); cur; cur = cur->next)
                    {
                        if (TInfo<T>::IsOverlap(cur->range, record.range))
                        {
                            previous.stage |= cur->stage;
                            previous.access |= cur->access;

                            if (previous.layout != VK_IMAGE_LAYOUT_UNDEFINED)
                            {
                                previous.layout = cur->layout;
                            }
                        }
                    }
                }

                if (previous.stage == VK_PIPELINE_STAGE_NONE)
                {
                    previous.stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                }

                if (previous.layout == VK_IMAGE_LAYOUT_MAX_ENUM)
                {
                    previous.layout = VK_IMAGE_LAYOUT_UNDEFINED;
                }

                return previous;
            }
            
            void TransferRecords(VulkanBarrierHandler* target);
            bool Resolve(VulkanBarrierInfo* outBarrierInfo);
            void Prune();

        private:
            template<typename T, typename TBarrier>
            void ProcessBarrier(const T resource, TBarrier** barrier, const AccessRecord& recordOld, const AccessRecord& recordNew);

            inline void Delete(AccessRecord** current, AccessRecord*** next)
            {
                auto deleted = *current;
                *current = **next;
                *next = current;
                m_records.Delete(deleted);
            }

            const uint32_t m_queueFamily = 0u;
            PK::Utilities::PointerMap<uint64_t, AccessRecord> m_resources;
            PK::Utilities::FixedPool<AccessRecord, 1024> m_records;
            PK::Utilities::FixedList<VkBufferMemoryBarrier, 256> m_bufferBarriers;
            PK::Utilities::FixedList<VkImageMemoryBarrier, 256> m_imageBarriers;
            PK::Utilities::Bitmask<1024> m_accessMask;
            PK::Utilities::Bitmask<1024> m_transferMask;
            VkPipelineStageFlags m_sourceStage = 0u;
            VkPipelineStageFlags m_destinationStage = 0u;
    };
}