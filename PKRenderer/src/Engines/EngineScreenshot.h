#pragma once
#include "Utilities/MemoryBlock.h"
#include "Core/Services/IService.h"
#include "Core/ControlFlow/IStepApplicationWindow.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/FenceRef.h"

namespace PK::Engines
{
    class EngineScreenshot : public Core::Services::IService,
        public Core::ControlFlow::IStepApplicationRenderWindow
    {
        public:
            EngineScreenshot();
            virtual void OnApplicationRender(Rendering::RHI::Window* window) final;

            void QueueCapture();

        private:
            Rendering::RHI::FenceRef m_copyFence;
            Utilities::MemoryBlock<uint16_t> m_accumulatedPixels;
            Rendering::RHI::Objects::BufferRef m_copyBuffer;
            uint32_t m_captureCounter = 0u;
            uint32_t m_captureFrameCount = 0u;
            Math::uint2 m_captureResolution = Math::PK_UINT2_ZERO;
            Math::uint2 m_currentResolution = Math::PK_UINT2_ZERO;
    };
}