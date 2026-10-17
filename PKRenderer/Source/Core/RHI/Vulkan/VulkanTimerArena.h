#pragma once
#include "Core/Base/Types/NameID.h"
#include "Core/Base/Containers/Mask.h"
#include "Core/Base/Containers/FixedArena.h"
#include "Core/Base/Containers/BufferView.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanTimerArena : public NoCopy
    {
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

        VulkanTimerArena(VkDevice device, float nanosecondsPerTick);

        uint32_t AddTimer(NameID name);
        void BeginTimeline();
        uint64_t EndTimeline();
        void FlushTimeline(uint64_t timelineIndex);
        ConstBufferView<RHITimerScope> GetResults() const;
        VkQueryPool GetQueryPool() const { return m_pool.pool; }

    private:
        const double m_ticksToSeconds;
        VulkanQueryPool m_pool;

        FixedMask<PK_VK_MAX_TIMER_SCOPES> m_timelineMask;
        TimelineScope m_timelines[PK_VK_MAX_TIMER_SCOPES];
        NameID m_scopeNames[PK_VK_MAX_TIMERS];
        RHITimerScope m_resolved[PK_VK_MAX_TIMERS];
        VkDeviceSize m_results[PK_VK_MAX_TIMER_QUERIES];
        
        uint64_t m_timelineHead = 0ull;
        uint64_t m_timerHead = 0ull;

        uint64_t m_timerFlushHead = 0ull;
        uint64_t m_timelineFlushHead = 0ull;
        bool m_inTimeline = false;
    };
}
