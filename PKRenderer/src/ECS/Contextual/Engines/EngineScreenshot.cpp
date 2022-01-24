#include "PrecompiledHeader.h"
#include "EngineScreenshot.h"
#include "Rendering/GraphicsAPI.h"

namespace PK::ECS::Engines
{
    using namespace PK::Rendering;

    // Source: https://elcharolin.wordpress.com/2018/11/28/read-and-write-bmp-files-in-c-c/
    static void WriteImage(const char* fileName, byte* pixels, uint width, uint height)
    {
        const uint BYTES_PER_PIXEL = 4u;
        const int DATA_OFFSET_OFFSET = 0x000A;
        const int WIDTH_OFFSET = 0x0012;
        const int HEIGHT_OFFSET = 0x0016;
        const int BITS_PER_PIXEL_OFFSET = 0x001C;
        const int HEADER_SIZE = 14;
        const int INFO_HEADER_SIZE = 40;
        const int NO_COMPRESION = 0;
        const int MAX_NUMBER_OF_COLORS = 0;
        const int ALL_COLORS_REQUIRED = 0;

        FILE* outputFile = fopen(fileName, "wb");
        //*****HEADER************//
        const char* BM = "BM";
        fwrite(&BM[0], 1, 1, outputFile);
        fwrite(&BM[1], 1, 1, outputFile);
        int paddedRowSize = (int)(4 * ceil((float)width / 4.0f)) * BYTES_PER_PIXEL;
        uint fileSize = paddedRowSize * height + HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&fileSize, 4, 1, outputFile);
        uint reserved = 0x0000;
        fwrite(&reserved, 4, 1, outputFile);
        uint dataOffset = HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&dataOffset, 4, 1, outputFile);

        //*******INFO*HEADER******//
        uint infoHeaderSize = INFO_HEADER_SIZE;
        fwrite(&infoHeaderSize, 4, 1, outputFile);
        fwrite(&width, 4, 1, outputFile);
        fwrite(&height, 4, 1, outputFile);
        ushort planes = 1; //always 1
        fwrite(&planes, 2, 1, outputFile);
        ushort bitsPerPixel = BYTES_PER_PIXEL * 8;
        fwrite(&bitsPerPixel, 2, 1, outputFile);
        //write compression
        uint compression = NO_COMPRESION;
        fwrite(&compression, 4, 1, outputFile);
        //write image size(in bytes)
        uint imageSize = width * height * BYTES_PER_PIXEL;
        fwrite(&imageSize, 4, 1, outputFile);
        uint resolutionX = 11811; //300 dpi
        uint resolutionY = 11811; //300 dpi
        fwrite(&resolutionX, 4, 1, outputFile);
        fwrite(&resolutionY, 4, 1, outputFile);
        uint colorsUsed = MAX_NUMBER_OF_COLORS;
        fwrite(&colorsUsed, 4, 1, outputFile);
        uint importantColors = ALL_COLORS_REQUIRED;
        fwrite(&importantColors, 4, 1, outputFile);
        int unpaddedRowSize = width * BYTES_PER_PIXEL;

        for (int32_t y = height - 1; y >= 0; --y)
        for (uint32_t x = 0u; x < width; ++x)
        {
            uint index = (x + y * width) * BYTES_PER_PIXEL;
            byte color[4] = { pixels[index + 0], pixels[index + 1], pixels[index + 2], pixels[index + 3] };
            fwrite(color, sizeof(byte), 4, outputFile);
        }

        fclose(outputFile);
    }

    EngineScreenshot::EngineScreenshot() : m_accumulatedPixels(1)
    {
    }
    
    void EngineScreenshot::Step(Window* window)
    {
        m_currentResolution = uint2(window->GetResolution().xy);
        
        if (m_currentResolution != m_captureResolution)
        {
            m_captureCounter = 0u;
            m_copyGate.Invalidate();
            return;
        }

        if (m_captureCounter == 0 || (m_copyGate.IsValid() && !m_copyGate.IsComplete()))
        {
            return;
        }

        auto elementCount = m_captureResolution.x * m_captureResolution.y * 4;

        auto cmd = GraphicsAPI::GetCommandBuffer();
    
        if (m_copyGate.IsValid() && m_copyGate.IsComplete())
        {
            auto pxView = m_copyBuffer->BeginRead<unsigned char>();
        
            for (auto i = 0u; i < elementCount; ++i)
            {
                m_accumulatedPixels[i] += pxView[i];
            }

            m_copyBuffer->EndRead();
        }

        m_captureCounter--;
        m_copyGate.Invalidate();

        if (m_captureCounter > 0)
        {
            cmd->Blit(window, m_copyBuffer.get());
            m_copyGate = cmd->GetOnCompleteGate();
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

        WriteImage(filename.c_str(), pixels, m_captureResolution.x, m_captureResolution.y);
        free(pixels);

        PK_LOG_INFO("Screenshot captured: %s", filename.c_str());
    }

    void EngineScreenshot::Step(ConsoleCommandToken* token)
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
            m_copyBuffer = Buffer::Create({ { ElementType::Uint, "DATA"} }, nullptr, m_currentResolution.x * m_currentResolution.y, BufferUsage::GPUToCPU | BufferUsage::TransferDst | BufferUsage::TransferSrc);
        }
        else
        {
            m_copyBuffer->Validate(m_currentResolution.x * m_currentResolution.y);
        }

        m_copyGate.Invalidate();
        m_accumulatedPixels.Validate(m_currentResolution.x * m_currentResolution.y * 4, true);
        m_accumulatedPixels.Clear();
        m_captureCounter = 9;
        m_captureFrameCount = 8;
    }
}