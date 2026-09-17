#pragma once
#include "Core/Base/Containers/FixedArena.h"
#include "Core/Base/Containers/BufferView.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanQueueTimer : public NoCopy
    {
        constexpr const static uint32_t MAX_TIMERS = 4096u;
        constexpr const static uint32_t MAX_QUERIES = MAX_TIMERS * 2ull;
        constexpr const static uint32_t MAX_TIMELINES = 64u;
        constexpr const static uint32_t MAX_STACK = 32u;

        struct TimelineScope
        {
            uint64_t first;
            uint64_t count;
        };

        struct TimerScope
        {
            VulkanQueryPool* pool;
            uint32_t query;
        };

        VulkanQueueTimer(VkDevice device, float nanosecondsPerTick);

        TimerScope Push(uint64_t userHash);
        TimerScope Pop();
        void BeginTimeline();
        uint64_t EndTimeline();
        void FlushTimeline(uint64_t timelineIndex);
        ConstBufferView<RHITimerScope> GetResults();

    private:
        const double m_ticksToSeconds;
        VulkanQueryPool m_pool;

        TimelineScope m_timelines[MAX_TIMELINES];
        uint64_t m_userHashes[MAX_TIMERS];
        RHITimerScope m_resolved[MAX_TIMERS];
        VkDeviceSize m_results[MAX_QUERIES];
        uint64_t* m_stack[MAX_STACK];
        
        uint64_t m_timelineHead = 0ull;
        uint64_t m_timerHead = 0ull;
        uint64_t m_stackHead = 0ull;

        uint64_t m_timerFlushHead = 0ull;
        uint64_t m_timelineFlushHead = 0ull;
        bool m_inTimeline = false;
    };
}
