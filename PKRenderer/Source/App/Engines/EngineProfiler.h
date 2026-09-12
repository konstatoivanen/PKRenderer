#pragma once
#include "Core/Base/Containers/ArrayList.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/GUI/GUIWidgets.h"

namespace PK { class AssetDatabase; }
namespace PK { struct GUI; }

namespace PK::App
{
    class EngineProfiler :
        public IStep<GUI*>,
        public IStep<TimeFramerateInfo*>
    {
        constexpr const static uint32_t MAX_SAMPLE_COUNT = 512u;

    public:
        EngineProfiler();

        virtual void Step(GUI* gui) final;
        virtual void Step(TimeFramerateInfo* framerate) final { m_framerate = *framerate; }

    private:
        TimeFramerateInfo m_framerate{};
        GUIWindow m_window;

        double m_samples[MAX_SAMPLE_COUNT]{};
        uint64_t m_sampleHead = 0ull;

        bool m_enabled = false;

    };
}
