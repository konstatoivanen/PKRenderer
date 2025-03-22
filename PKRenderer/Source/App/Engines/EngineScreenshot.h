#pragma once
#include "Core/Utilities/MemoryBlock.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/ControlFlow/IStepApplication.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    class EngineScreenshot : public IStepApplicationRender<RHIWindow*>
    {
    public:
        EngineScreenshot();
        virtual void OnApplicationRender(RHIWindow* window) final;

        void QueueCapture();

    private:
        RHIBufferRef m_copyBuffer;
        MemoryBlock<uint16_t> m_accumulatedPixels;
        FenceRef m_copyFence;
        uint32_t m_captureCounter = 0u;
        uint32_t m_captureFrameCount = 0u;
        uint2 m_captureResolution = PK_UINT2_ZERO;
        uint2 m_currentResolution = PK_UINT2_ZERO;
    };
}