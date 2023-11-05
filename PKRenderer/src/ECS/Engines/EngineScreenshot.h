#pragma once
#include "Utilities/MemoryBlock.h"
#include "Core/ConsoleCommandBinding.h"
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::ECS::Engines
{
    class EngineScreenshot : public Core::Services::IService,
        public Core::Services::IStep<Rendering::RHI::Window>,
        public Core::Services::IStep<Core::TokenConsoleCommand>
    {
    public:
        EngineScreenshot();
        void Step(Rendering::RHI::Window* window) final;
        void Step(Core::TokenConsoleCommand* token) final;

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