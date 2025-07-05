#pragma once
#include "Core/Utilities/NoCopy.h"
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FixedList.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    constexpr static const uint8_t PK_RHI_ACCESS_OPT_BARRIER = 1 << 0;
    constexpr static const uint8_t PK_RHI_ACCESS_OPT_TRANSFER = 1 << 1;
    constexpr static const uint8_t PK_RHI_ACCESS_OPT_DEFAULT = 1 << 2;

    class VulkanBarrierHandler : public NoCopy
    {
        public:
            struct AccessRecord
            {
                union
                {
                    uint64_t range = 0u;
                    TextureViewRange imageRange;

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
                static void SetDefaultRange([[maybe_unused]] AccessRecord* a) {};
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
                uint32_t index = 0u;
                auto key = reinterpret_cast<uint64_t>(resource);

                if (m_resources.AddKey(key, &index))
                {
                    options |= PK_RHI_ACCESS_OPT_DEFAULT;
                }

                Record(resource, index, record, options);
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
                    for (auto cur = m_resources[index].value; cur; cur = cur->next)
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
            void ClearBarriers();
            void Prune();

        private:
            template<typename T>
            void Record(const T resource, uint32_t index, const AccessRecord& record, uint8_t options)
            {
                typedef typename TInfo<T>::BarrierType TBarrier;
                TBarrier* barrier = nullptr;

                auto scope = record;
                scope.next = nullptr;

                if (options & PK_RHI_ACCESS_OPT_DEFAULT)
                {
                    auto defaultRecord = m_records.New(scope);
                    TInfo<T>::SetDefaultRange(defaultRecord);
                    m_resources[index].value = defaultRecord;
                    m_resolveTimestamps[index] = 0ull;
                    m_accessTimestamps[index] = 0ull;
                    index = m_resources.GetCount() - 1;
                }

                auto isSequentialAccess = true;

                if ((options & PK_RHI_ACCESS_OPT_TRANSFER) == 0u)
                {
                    isSequentialAccess = (m_globalResolveCounter - m_resolveTimestamps[index]) < 2u;
                    m_resolveTimestamps[index] = m_globalResolveCounter;
                    m_accessTimestamps[index] = m_globalAccessCounter++;
                }

                auto current = &m_resources[index].value;

                for (auto next = &(*current)->next; *current; current = next, next = &(*current)->next)
                {
                    if (TInfo<T>::IsInclusive((*current)->range, scope.range) &&
                        (*current)->stage == scope.stage &&
                        (*current)->access == scope.access &&
                        (*current)->layout == scope.layout &&
                        (*current)->queueFamily == scope.queueFamily &&
                        !(VulkanEnumConvert::IsWriteAccess(scope.access) && isSequentialAccess))
                    {
                        return;
                    }

                    auto writeCur = VulkanEnumConvert::IsWriteAccess((*current)->access);
                    auto writeNew = VulkanEnumConvert::IsWriteAccess(scope.access);
                    auto overlap = TInfo<T>::IsOverlap((*current)->range, scope.range);
                    auto adjacent = TInfo<T>::IsAdjacent((*current)->range, scope.range);
                    auto mergeFlags = (*current)->layout == scope.layout && (*current)->queueFamily == scope.queueFamily;
                    auto mergeOverlap = overlap && !writeCur && !writeNew;
                    auto mergeAdjacent = adjacent && writeCur == writeNew;
                    auto skipBarrier = !isSequentialAccess && mergeFlags;

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

                    // If accesses are padded by at least one command in the same queue we can omit a barrier.
                    if ((options & PK_RHI_ACCESS_OPT_BARRIER) && !skipBarrier)
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
                        slice->range = ranges[i];
                        slice->next = (*current)->next;
                        next = &slice->next;

                        (*current)->next = slice;
                        current = &(*current)->next;
                    }
                }

                *current = m_records.New(scope);
            }

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
            FixedPointerMap16<uint64_t, AccessRecord, 1024u> m_resources;
            FixedPool<AccessRecord, 1024> m_records;
            FixedList<VkBufferMemoryBarrier, 256> m_bufferBarriers;
            FixedList<VkImageMemoryBarrier, 256> m_imageBarriers;
            uint64_t m_accessTimestamps[1024]{};
            uint64_t m_resolveTimestamps[1024]{};
            VkPipelineStageFlags m_sourceStage = 0u;
            VkPipelineStageFlags m_destinationStage = 0u;
            uint64_t m_pruneTimeStamp = 0ull;
            inline static uint64_t m_globalAccessCounter = 0ull;
            inline static uint64_t m_globalResolveCounter = 0ull;
    };
}