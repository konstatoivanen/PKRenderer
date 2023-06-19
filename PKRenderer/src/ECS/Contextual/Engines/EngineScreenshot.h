#pragma once
#include "Core/Services/IService.h"
#include "Core/Services/Sequencer.h"
#include "Core/Window.h"
#include "Core/ConsoleCommandBinding.h"
#include "Utilities/MemoryBlock.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Structs/FenceRef.h"

namespace PK::ECS::Engines
{
    class EngineScreenshot : public Core::Services::IService,
        public Core::Services::IStep<Core::Window>,
        public Core::Services::IStep<Core::TokenConsoleCommand>
    {
    public:
        EngineScreenshot();
        void Step(Core::Window* window) final;
        void Step(Core::TokenConsoleCommand* token) final;

    private:
        Rendering::Structs::FenceRef m_copyFence;
        Utilities::MemoryBlock<uint16_t> m_accumulatedPixels;
        Utilities::Ref<Rendering::Objects::Buffer> m_copyBuffer;
        uint32_t m_captureCounter = 0u;
        uint32_t m_captureFrameCount = 0u;
        Math::uint2 m_captureResolution = Math::PK_UINT2_ZERO;
        Math::uint2 m_currentResolution = Math::PK_UINT2_ZERO;
    };
}