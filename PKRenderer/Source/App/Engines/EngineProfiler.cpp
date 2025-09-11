#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedMask.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Math/FunctionsColor.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "App/Renderer/IGUIRenderer.h"
#include "EngineProfiler.h"

namespace PK::App
{
    EngineProfiler::EngineProfiler(AssetDatabase* assetDatabase) :
        m_assetDatabase(assetDatabase)
    {
    }
    
    void EngineProfiler::Step(IGUIRenderer* gui)
    {
        constexpr auto COLOR_BG = color32(0, 0, 0, 192);
        constexpr auto COLOR_FG = color32(255, 255, 255, 64);
        constexpr auto COLOR_FPS_AVG = color32(255, 255, 255, 127);
        constexpr auto COLOR_FPS_MIN = color32(0, 255, 0, 127);
        constexpr auto COLOR_FPS_MAX = color32(255, 0, 0, 127);

        // @TODO this is pretty hacky & hard coded. fix later
        const auto renderArea = gui->GUIGetRenderAreaRect();
        const auto rectWindow = short4(renderArea.x + 4, renderArea.y + 78, renderArea.z - 8, -74);
        const auto rectSample = short4(rectWindow.x + 8, rectWindow.y + rectWindow.w + 7, 2, 1);
        const auto rectBar = short4(rectWindow.x + 8, rectSample.y, rectWindow.z - 16, 1);
        const auto sampleWidth = 2;
        const auto sampleHeight = 38;
        const auto sampleCountMax = (uint64_t)(rectBar.z / sampleWidth);
        const auto sampleCountMin = glm::min(sampleCountMax, m_timeHistoryHead + 1ull);

        m_timeHistory.resize(sampleCountMax);
        m_timeHistory.at(m_timeHistoryHead % sampleCountMax) = m_framerate.frameMs;

        auto minHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMax);
        auto maxHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMin);
        auto avgHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateAvg);

        for (auto i = 0ull; i < sampleCountMin; ++i)
        {
            minHistoryTime = glm::min(minHistoryTime, m_timeHistory.at(i));
            maxHistoryTime = glm::max(maxHistoryTime, m_timeHistory.at(i));
        }

        FixedString64 textFramerate("FPS: %i", m_framerate.framerate);
        FixedString64 textFramerateAvg("Avg: %4.2fms", avgHistoryTime);
        FixedString64 textFramerateMin("Min: %4.2fms", minHistoryTime);
        FixedString64 textFramerateMax("Max: %4.2fms", maxHistoryTime);

        gui->GUIDrawRect(COLOR_BG, rectWindow);
        gui->GUIDrawWireRect(COLOR_FG, rectWindow, 1);
        gui->GUIDrawText(COLOR_FPS_AVG, rectWindow.xy + short2(12 + 100 * 0, -3), textFramerate.c_str(), 16.0f);
        gui->GUIDrawText(COLOR_FPS_AVG, rectWindow.xy + short2(12 + 100 * 1, -3), textFramerateAvg.c_str(), 16.0f);
        gui->GUIDrawText(COLOR_FPS_MIN, rectWindow.xy + short2(12 + 100 * 2, -3), textFramerateMin.c_str(), 16.0f);
        gui->GUIDrawText(COLOR_FPS_MAX, rectWindow.xy + short2(12 + 100 * 3, -3), textFramerateMax.c_str(), 16.0f);

        for (auto i = 0ull; i < sampleCountMin; ++i)
        {
            const auto offset = (i + sampleCountMax - sampleCountMin) * sampleWidth;
            const auto sample = m_timeHistory.at((m_timeHistoryHead + i + 1ull) % sampleCountMin);
            const auto normalized = (sample - minHistoryTime) / (maxHistoryTime - minHistoryTime);
            const auto height = (int)glm::round(sampleHeight * normalized);
            const auto color = Math::HueToRGB32((1.0f - normalized) / 3.0f);
            gui->GUIDrawRect(color32(color.r, color.g, color.b, 127), rectSample + short4(offset, 0, 0, height));
        }

        gui->GUIDrawRect(COLOR_FPS_MIN, rectBar + short4(0, sampleHeight * 0, 0, 0));
        gui->GUIDrawRect(COLOR_FPS_AVG, rectBar + short4(0, sampleHeight / 2, 0, 0));
        gui->GUIDrawRect(COLOR_FPS_MAX, rectBar + short4(0, sampleHeight * 1, 0, 0));

        m_timeHistoryHead++;
    }
}
