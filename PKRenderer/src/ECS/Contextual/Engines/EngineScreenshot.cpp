#include "PrecompiledHeader.h"
#include "EngineScreenshot.h"
#include "Rendering/GraphicsAPI.h"
#include "Utilities/FileIOBMP.h"

namespace PK::ECS::Engines
{
    using namespace Math;
    using namespace Core;
    using namespace Core::Services;
    using namespace Rendering;
    using namespace Rendering::Structs;
    using namespace Rendering::Objects;

    EngineScreenshot::EngineScreenshot() : m_accumulatedPixels(1)
    {
    }

    void EngineScreenshot::Step(Window* window)
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

        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Graphics);

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

    void EngineScreenshot::Step(TokenConsoleCommand* token)
    {
        if (token->isConsumed || token->argument != "take_screenshot")
        {
            return;
        }

        token->isConsumed = true;

        if (m_captureCounter > 0 || m_currentResolution.x == 0 || m_currentResolution.y == 0)
        {
            return;
        }

        m_captureResolution = m_currentResolution;

        if (m_copyBuffer == nullptr)
        {
            m_copyBuffer = Buffer::Create(
                { { ElementType::Uint, "DATA"} },
                m_currentResolution.x * m_currentResolution.y,
                BufferUsage::GPUToCPU | BufferUsage::TransferDst | BufferUsage::TransferSrc,
                "Screenshot Copy Buffer");
        }
        else
        {
            m_copyBuffer->Validate(m_currentResolution.x * m_currentResolution.y);
        }

        m_copyFence.Invalidate();
        m_accumulatedPixels.Validate(m_currentResolution.x * m_currentResolution.y * 4, true);
        m_accumulatedPixels.Clear();
        m_captureCounter = 9;
        m_captureFrameCount = 8;
    }
}