#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Utilities/FixedPool.h"
#include "Utilities/FixedList.h"
#include "Utilities/PointerMap.h"

namespace PK::Rendering::VulkanRHI::Services
{
    class VulkanBarrierHandler : PK::Utilities::NoCopy
    {
            template<typename T> struct BarrierType {};
            template<> struct BarrierType<VkBuffer> { using Type = VkBufferMemoryBarrier; };
            template<> struct BarrierType<VkImage> { using Type = VkImageMemoryBarrier; };

            template<typename T>
            inline static void ConditionalCopy(T* dst, const T& src)
            {
                if (dst)
                {
                    *dst = src;
                }
            }

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
                VkImageAspectFlags aspect = 0u;
                VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
                AccessRecord* next = nullptr;

                AccessRecord() {};
                AccessRecord(const VkBuffer& buffer, const AccessRecord& record) :
                    stage(record.stage),
                    access(record.access),
                    bufferRange(record.bufferRange),
                    next(nullptr)
                {
                }

                AccessRecord(const VkImage& buffer, const AccessRecord& record) :
                    stage(record.stage),
                    access(record.access),
                    layout(record.layout),
                    imageRange(record.imageRange),
                    next(nullptr)
                {
                }

                template<typename T>
                void Set(const AccessRecord& record);
                template<typename T>
                void Merge(const AccessRecord& record);
                template<typename T>
                bool IsOverlap(const AccessRecord& record) const;
                template<typename T>
                bool IsInclusive(const AccessRecord& record) const;
                template<typename T>
                bool IsAccessDiff(const AccessRecord& record) const;
            };

            VulkanBarrierHandler() = default;
            ~VulkanBarrierHandler();

            template<typename T>
            void Record(const T resource, AccessRecord record, AccessRecord* outValue = nullptr, bool writeBarrier = true)
            {
                auto key = reinterpret_cast<uint64_t>(resource);
                auto index = m_resources.GetIndex(key);

                // First value no resolve
                if (index == -1)
                {
                    m_resources.AddValue(key, m_records.New(resource, record));
                    m_pruneTicks.push_back(m_currentPruneTick + 1ull);
                    ConditionalCopy(outValue, record);
                    return;
                }

                typedef BarrierType<T>::Type TBarrier;
                TBarrier* barrier = nullptr;

                AccessRecord** current = m_resources.GetValueAtRef(index);
                m_pruneTicks.at(index) = m_currentPruneTick + 1ull;

                while (current && *current)
                {
                    if ((*current)->IsInclusive<T>(record))
                    {
                        ConditionalCopy(outValue, record);
                        return;
                    }

                    auto overlap = (*current)->IsOverlap<T>(record);
                    auto accessDiff = (*current)->IsAccessDiff<T>(record);

                    // Resolve
                    if (overlap && accessDiff)
                    {
                        if (writeBarrier)
                        {
                            ProcessBarrier<T>(resource, &barrier, **current, record);
                        }
                       
                        if ((*current)->next == nullptr)
                        {
                            ConditionalCopy(outValue, **current);
                            (*current)->Set<T>(record);
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
                            ConditionalCopy(outValue, **current);
                            (*current)->Merge<T>(record);
                            return;
                        }

                        record.Merge<T>(**current);
                        *current = RemoveRecord(*current);
                        continue;
                    }

                    // Append
                    if ((*current)->next == nullptr)
                    {
                        (*current)->next = m_records.New(resource, record);
                        ConditionalCopy(outValue, *(*current)->next);
                        return;
                    }

                    current = &(*current)->next;
                }
            }

            bool Resolve(VulkanBarrierInfo* outBarrierInfo);
            void Prune();

        private:
            template<typename T, typename TBarrier>
            void ProcessBarrier(const T resource, TBarrier** barrier, const AccessRecord& recordOld, const AccessRecord& recordNew);
            AccessRecord* RemoveRecord(AccessRecord* record);

            PK::Utilities::PointerMap<uint64_t, AccessRecord> m_resources;
            PK::Utilities::FixedPool<AccessRecord, 1024> m_records;
            PK::Utilities::FixedList<VkBufferMemoryBarrier, 1024> m_bufferBarriers;
            PK::Utilities::FixedList<VkImageMemoryBarrier, 1024> m_imageBarriers;
            std::vector<uint64_t> m_pruneTicks;
            VkPipelineStageFlags m_sourceStage = 0u;
            VkPipelineStageFlags m_destinationStage = 0u;
            uint64_t m_currentPruneTick = 0u;
    };
}