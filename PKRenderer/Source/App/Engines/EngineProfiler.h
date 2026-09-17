#pragma once
#include "Core/Base/Containers/ArrayList.h"
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Types/NameID.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/Timers/TimeFrameInfo.h"
#include "Core/GUI/GUIWidgets.h"

namespace PK { class AssetDatabase; }
namespace PK { struct GUI; }

namespace PK::App
{
    struct EngineProfiler :
        public IStep<GUI*>,
        public IStep<TimeFramerateInfo*>
    {
        struct NamedTimer
        {
            double elapsed;
            uint64_t tickIndex;
        };

        constexpr const static uint32_t MAX_SAMPLE_COUNT = 512u;

        EngineProfiler();

        virtual void Step(GUI* gui) final;
        virtual void Step(TimeFramerateInfo* framerate) final { m_framerate = *framerate; }

    private:
        FixedMap<NameID, NamedTimer, 1024u> m_rhiTimers;
        TimeFramerateInfo m_framerate{};
        GUIWindow m_window;

        double m_samples[MAX_SAMPLE_COUNT]{};
        uint64_t m_sampleHead = 0ull;

        bool m_enabled = false;
    };
}
