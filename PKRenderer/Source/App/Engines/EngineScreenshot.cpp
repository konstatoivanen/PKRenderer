#include "PrecompiledHeader.h"
#include <filesystem>
#include "Core/Utilities/Parse.h"
#include "Core/Utilities/FileIOBMP.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/Window.h"
#include "App/FrameContext.h"
#include "EngineScreenshot.h"

namespace PK::App
{
    EngineScreenshot::EngineScreenshot() : 
        m_accumulatedPixels(0)
    {
        CVariableRegister::Create<CVariableFuncSimple>("Engine.Screenshot.QueueCapture", [this](){QueueCapture();});
    }

    void EngineScreenshot::OnStepFrameRender(FrameContext* ctx)
    {
        auto window = ctx->window;

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

        auto cmd = RHI::GetCommandBuffer(QueueType::Graphics);

        if (m_copyFence.IsValid() && m_copyFence.IsComplete())
        {
            auto pxView = m_copyBuffer->BeginRead<unsigned char>();

            for (auto i = 0u; i < elementCount; ++i)
            {
                m_accumulatedPixels[i] += pxView[i];
            }

            m_copyBuffer->EndMap(0ull, 0ull);
        }

        m_captureCounter--;
        m_copyFence.Invalidate();

        if (m_captureCounter > 0)
        {
            cmd->Blit(window->GetSwapchain(), m_copyBuffer.get());
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

        FixedString32 filename("Screenshot0.bmp");
        auto index = 0;

        while (std::filesystem::exists(filename.c_str()))
        {
            filename = FixedString32("Screenshot%i.bmp", ++index);
        }

        PK::FileIO::WriteBMP(filename.c_str(), pixels, m_captureResolution.x, m_captureResolution.y);
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

        auto usage = BufferUsage::GPUToCPU | BufferUsage::TransferDst | BufferUsage::TransferSrc;
        RHI::ValidateBuffer<uint32_t>(m_copyBuffer, m_currentResolution.x * m_currentResolution.y, usage, "Screenshot.CopyBuffer");

        m_copyFence.Invalidate();
        m_accumulatedPixels.Validate(m_currentResolution.x * m_currentResolution.y * 4);
        m_accumulatedPixels.Clear();
        m_captureCounter = 9;
        m_captureFrameCount = 8;
    }
}
