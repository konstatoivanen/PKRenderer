#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/ControlFlow/IStep.h"
#include "Core/Timers/TimeFrameInfo.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    struct IGUIRenderer;

    class EngineProfiler :
        public IStep<IGUIRenderer*>,
        public IStep<TimeFramerateInfo*>
    {
    public:
        EngineProfiler(AssetDatabase* assetDatabase);

        void AnalyzeShaderResourceLayouts();

        virtual void Step(IGUIRenderer* gui) final;
        virtual void Step(TimeFramerateInfo* framerate) final { m_framerate = *framerate; }

    private:
        AssetDatabase* m_assetDatabase = nullptr;
        TimeFramerateInfo m_framerate{};
        std::vector<double> m_timeHistory;
        uint32_t m_timeHistoryHead = 0u;
    };
}