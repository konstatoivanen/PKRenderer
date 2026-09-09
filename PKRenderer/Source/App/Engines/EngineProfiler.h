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
    public:
        EngineProfiler();

        virtual void Step(GUI* gui) final;
        virtual void Step(TimeFramerateInfo* framerate) final { m_framerate = *framerate; }

    private:
        TimeFramerateInfo m_framerate{};
        HeapArray<double> m_timeHistory;
        uint64_t m_timeHistoryHead = 0ull;
        bool m_enabled = false;

        GUIWindow m_window{ GUIWindowStyle::GetRed(short2(300, 420), PK_FLOAT2_RIGHT) };
    };
}
