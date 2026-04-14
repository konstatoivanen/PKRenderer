#pragma once
#include "Core/Utilities/FastBuffer.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/Timers/TimeFrameInfo.h"

namespace PK { class AssetDatabase; }

namespace PK::App
{
    struct IGUIRenderer;

    class EngineProfiler :
        public IStep<IGUIRenderer*>,
        public IStep<TimeFramerateInfo*>
    {
    public:
        EngineProfiler(AssetDatabase* assetDatabase);

        virtual void Step(IGUIRenderer* gui) final;
        virtual void Step(TimeFramerateInfo* framerate) final { m_framerate = *framerate; }

    private:
        AssetDatabase* m_assetDatabase = nullptr;
        TimeFramerateInfo m_framerate{};
        FastBuffer<double> m_timeHistory;
        uint64_t m_timeHistoryHead = 0ull;
        bool m_enabled = false;
    };
}
