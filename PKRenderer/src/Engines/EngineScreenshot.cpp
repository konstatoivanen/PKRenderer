#include "PrecompiledHeader.h"
#include <filesystem>
#include "Utilities/FileIOBMP.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Window.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "EngineScreenshot.h"

namespace PK::Engines
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::CLI;
    using namespace PK::Rendering;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    EngineScreenshot::EngineScreenshot() : m_accumulatedPixels(1)
    {
        CVariableRegister::Create<CVariableFunc>("Engine.Screenshot.QueueCapture", [this](const char** args, uint32_t count) { QueueCapture(); });
    }

    void EngineScreenshot::OnApplicationRender(Window* window)
    {
        m_currentResolution = uint2(window->GetResolution().xy);

        if (m_currentResolution != m_captureResolution)
        {
            m_captureCounter = 0u;
            m_copyFence.Invalidate();
            return;
        }

        if (m_captureCounter == 0 || (m_copyFence.IsValid() && !m_copyFence.IsComplete()))
        {
            return;
        }

        auto elementCount = m_captureResolution.x * m_captureResolution.y * 4;

        auto cmd = GraphicsAPI::GetCommandBuffer(QueueType::Graphics);

        if (m_copyFence.IsValid() && m_copyFence.IsComplete())
        {
            auto pxView = m_copyBuffer->BeginRead<unsigned char>();

            for (auto i = 0u; i < elementCount; ++i)
            {
                m_accumulatedPixels[i] += pxView[i];
            }

            m_copyBuffer->EndRead();
        }

        m_captureCounter--;
        m_copyFence.Invalidate();

        if (m_captureCounter > 0)
        {
            cmd->Blit(window, m_copyBuffer.get());
            m_copyFence = cmd->GetFenceRef();
            return;
        }

        auto pixels = PK_CONTIGUOUS_ALLOC(byte, elementCount);

        for (auto i = 0u; i < elementCount; ++i)
        {
            auto value = m_accumulatedPixels[i] / m_captureFrameCount;

            if (value > 255)
            {
                value = 255;
            }

            pixels[i] = value;
        }

        auto filename = std::string("Screenshot0.bmp");
        auto index = 0;

        while (std::filesystem::exists(filename))
        {
            filename = std::string("Screenshot") + std::to_string(++index) + std::string(".bmp");
        }

        Utilities::FileIO::WriteBMP(filename.c_str(), pixels, m_captureResolution.x, m_captureResolution.y);
        free(pixels);

        PK_LOG_INFO("Screenshot captured: %s", filename.c_str());
    }

    void EngineScreenshot::QueueCapture()
    {
        if (m_captureCounter > 0 || m_currentResolution.x == 0 || m_currentResolution.y == 0)
        {
            return;
        }

        m_captureResolution = m_currentResolution;

        if (m_copyBuffer == nullptr)
        {
            m_copyBuffer = Buffer::Create<uint32_t>(m_currentResolution.x * m_currentResolution.y,
                BufferUsage::GPUToCPU | BufferUsage::TransferDst | BufferUsage::TransferSrc,
                "Screenshot.CopyBuffer");
        }
        else
        {
            m_copyBuffer->Validate<uint32_t>(m_currentResolution.x * m_currentResolution.y);
        }

        m_copyFence.Invalidate();
        m_accumulatedPixels.Validate(m_currentResolution.x * m_currentResolution.y * 4, true);
        m_accumulatedPixels.Clear();
        m_captureCounter = 9;
        m_captureFrameCount = 8;
    }
}