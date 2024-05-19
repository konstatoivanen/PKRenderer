#pragma once
#include "Utilities/MemoryBlock.h"
#include "Utilities/FenceRef.h"
#include "Math/Types.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Core/IService.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Engines
{
    class EngineScreenshot : public Core::IService,
        public Core::ControlFlow::IStepApplicationRenderWindow
    {
    public:
        EngineScreenshot();
        virtual void OnApplicationRender(Graphics::Window* window) final;

        void QueueCapture();

    private:
        Utilities::FenceRef m_copyFence;
        Utilities::MemoryBlock<uint16_t> m_accumulatedPixels;
        Graphics::BufferRef m_copyBuffer;
        uint32_t m_captureCounter = 0u;
        uint32_t m_captureFrameCount = 0u;
        Math::uint2 m_captureResolution = Math::PK_UINT2_ZERO;
        Math::uint2 m_currentResolution = Math::PK_UINT2_ZERO;
    };
}