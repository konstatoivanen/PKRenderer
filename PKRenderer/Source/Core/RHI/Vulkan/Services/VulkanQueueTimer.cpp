#include "PrecompiledHeader.h"
#include "VulkanQueueTimer.h"

namespace PK
{
    VulkanQueueTimer::VulkanQueueTimer(VkDevice device, float nanosecondsPerTick) :
        m_ticksToSeconds(static_cast<double>(nanosecondsPerTick) * 1e-9),
        m_pool(device, VK_QUERY_TYPE_TIMESTAMP, MAX_QUERIES)
    {
    }

    bool VulkanQueueTimer::Push(NameID name, uint32_t* outIndex)
    {
        if (m_inTimeline && m_stackHead < MAX_STACK && 
            m_timerHead - m_timerFlushHead < MAX_TIMERS)
        {
            const auto index = static_cast<uint32_t>(m_timerHead % MAX_TIMERS);
            m_scopeNames[index] = name;
            m_stack[m_stackHead++] = &m_scopeNames[index];
            ++m_timerHead;

            *outIndex = index * 2u + 0u;
            return true;
        }

        return false;
    }

    bool VulkanQueueTimer::Pop(uint32_t* outIndex)
    {
        if (m_inTimeline && m_stackHead)
        {
            *outIndex = static_cast<uint32_t>(m_stack[--m_stackHead] - &m_scopeNames[0]) * 2u + 1u;
            return true;
        }

        return false;
    }

    void VulkanQueueTimer::BeginTimeline()
    {
        if (!m_inTimeline && m_timelineHead - m_timelineFlushHead < MAX_TIMELINES)
        {
            m_inTimeline = true;
            auto& timeline = m_timelines[m_timelineHead % MAX_TIMELINES];
            timeline.first = m_timerHead;
            timeline.count = 0ull;
        }
    }

    uint64_t VulkanQueueTimer::EndTimeline()
    {
        auto index = m_timelineHead;

        if (m_inTimeline)
        {
            auto& timeline = m_timelines[m_timelineHead % MAX_TIMELINES];
            timeline.count = m_timerHead - timeline.first;
            m_inTimeline = false;
            ++m_timelineHead;
        }

        return index;
    }

    void VulkanQueueTimer::FlushTimeline(uint64_t timelineIndex)
    {
        if (timelineIndex >= m_timelineFlushHead)
        {
            const auto& timeline = m_timelines[timelineIndex % MAX_TIMELINES];

            if (timeline.count)
            {
                const auto queryFirst = static_cast<uint32_t>(timeline.first % MAX_TIMERS) * 2u;
                const auto queryCount = static_cast<uint32_t>(timeline.count) * 2u;

                const auto queryCount0 = queryFirst + queryCount > MAX_QUERIES ? MAX_QUERIES - queryFirst : queryCount;
                const auto queryCount1 = queryCount - queryCount0;

                if (queryCount0)
                {
                    m_pool.GetResults<VkDeviceSize>(m_results, queryFirst, queryCount0, VK_QUERY_RESULT_64_BIT);
                    m_pool.ResetQuery(queryFirst, queryCount0);
                }

                if (queryCount1)
                {
                    m_pool.GetResults<VkDeviceSize>(&m_results[0] + queryCount0, 0, queryCount1, VK_QUERY_RESULT_64_BIT);
                    m_pool.ResetQuery(0, queryCount1);
                }

                for (auto i = 0u; i < timeline.count; ++i)
                {
                    const auto index = static_cast<uint32_t>((timeline.first + i) % MAX_TIMERS);
                    const auto tickBeg = m_results[i * 2u + 0u];
                    const auto tickEnd = m_results[i * 2u + 1u];
                    const auto elapsed = tickEnd >= tickBeg ? tickEnd - tickBeg : 0ull;
                    m_resolved[index].name = m_scopeNames[index];
                    m_resolved[index].timerIndex = timeline.first + i;
                    m_resolved[index].elapsedSeconds = static_cast<double>(elapsed) * m_ticksToSeconds;
                }
            }

            if (timelineIndex == m_timelineFlushHead)
            {
                m_timerFlushHead += timeline.count;
                ++m_timelineFlushHead;
            }
        }
    }

    ConstBufferView<RHITimerScope> VulkanQueueTimer::GetResults() const
    {
        return { m_resolved, m_timerFlushHead > MAX_TIMERS ? MAX_TIMERS : m_timerFlushHead };
    }
}
