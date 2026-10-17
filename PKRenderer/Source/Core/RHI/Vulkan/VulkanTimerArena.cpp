#include "PrecompiledHeader.h"
#include "VulkanTimerArena.h"

namespace PK
{
    VulkanTimerArena::VulkanTimerArena(VkDevice device, float nanosecondsPerTick) :
        m_ticksToSeconds(static_cast<double>(nanosecondsPerTick) * 1e-9),
        m_pool(device, VK_QUERY_TYPE_TIMESTAMP, PK_VK_MAX_TIMER_QUERIES)
    {
    }

    uint32_t VulkanTimerArena::AddTimer(NameID name)
    {
        if (m_inTimeline && m_timerHead - m_timerFlushHead < PK_VK_MAX_TIMERS)
        {
            const auto index = static_cast<uint32_t>(m_timerHead % PK_VK_MAX_TIMERS);
            m_scopeNames[index] = name;
            ++m_timerHead;
            return index * 2u;
        }
        
        return ~0u;
    }

    void VulkanTimerArena::BeginTimeline()
    {
        if (!m_inTimeline && m_timelineHead - m_timelineFlushHead < PK_VK_MAX_TIMER_SCOPES)
        {
            m_inTimeline = true;
            auto& timeline = m_timelines[m_timelineHead % PK_VK_MAX_TIMER_SCOPES];
            timeline.first = m_timerHead;
            timeline.count = 0ull;
        }
    }

    uint64_t VulkanTimerArena::EndTimeline()
    {
        auto index = m_timelineHead;

        if (m_inTimeline)
        {
            auto& timeline = m_timelines[m_timelineHead % PK_VK_MAX_TIMER_SCOPES];
            timeline.count = m_timerHead - timeline.first;
            m_inTimeline = false;
            ++m_timelineHead;
        }

        return index;
    }

    void VulkanTimerArena::FlushTimeline(uint64_t timelineIndex)
    {
        if (timelineIndex >= m_timelineFlushHead)
        {
            const auto& timeline = m_timelines[timelineIndex % PK_VK_MAX_TIMER_SCOPES];

            if (timeline.count)
            {
                const auto queryFirst = static_cast<uint32_t>(timeline.first % PK_VK_MAX_TIMERS) * 2u;
                const auto queryCount = static_cast<uint32_t>(timeline.count) * 2u;

                const auto queryCount0 = queryFirst + queryCount > PK_VK_MAX_TIMER_QUERIES ? PK_VK_MAX_TIMER_QUERIES - queryFirst : queryCount;
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
                    const auto index = static_cast<uint32_t>((timeline.first + i) % PK_VK_MAX_TIMERS);
                    const auto tickBeg = m_results[i * 2u + 0u];
                    const auto tickEnd = m_results[i * 2u + 1u];
                    const auto elapsed = tickEnd >= tickBeg ? tickEnd - tickBeg : 0ull;
                    m_resolved[index].name = m_scopeNames[index];
                    m_resolved[index].timerIndex = timeline.first + i;
                    m_resolved[index].elapsedSeconds = static_cast<double>(elapsed) * m_ticksToSeconds;
                }
            }

            m_timelineMask[timelineIndex % PK_VK_MAX_TIMER_SCOPES] = true;

            // Increment out of order counters
            if (timelineIndex == m_timelineFlushHead)
            {
                while (m_timelineMask[m_timelineFlushHead % PK_VK_MAX_TIMER_SCOPES])
                {
                    m_timelineMask[m_timelineFlushHead % PK_VK_MAX_TIMER_SCOPES] = false;
                    m_timerFlushHead += m_timelines[m_timelineFlushHead % PK_VK_MAX_TIMER_SCOPES].count;
                    ++m_timelineFlushHead;
                }
            }
        }
    }

    ConstBufferView<RHITimerScope> VulkanTimerArena::GetResults() const
    {
        return { m_resolved, m_timerFlushHead > PK_VK_MAX_TIMERS ? PK_VK_MAX_TIMERS : m_timerFlushHead };
    }
}
