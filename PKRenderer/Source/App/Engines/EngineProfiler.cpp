#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedMask.h"
#include "Core/Utilities/FixedString.h"
#include "Core/Math/FunctionsColor.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Font.h"
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
        constexpr auto COLOR_FG = color32(255, 255, 255, 127);
        constexpr auto COLOR_FPS_AVG = color32(255, 255, 255, 127);
        constexpr auto COLOR_FPS_MIN = color32(0, 255, 0, 127);
        constexpr auto COLOR_FPS_MAX = color32(255, 0, 0, 127);

        // @TODO this is pretty hacky & hard coded. fix later
        const auto height = 74;
        const auto fontSize = 16;
        const auto padding = 4;
        const auto renderArea = gui->GUIGetRenderAreaRect();
        const auto rectWindow = short4(renderArea.x + padding, renderArea.w - height - padding, renderArea.z - padding * 2, height);
        const auto rectSample = short4(rectWindow.x + padding, rectWindow.y + rectWindow.w - padding, 2, 0);
        const auto rectBar    = short4(rectWindow.x + padding, rectWindow.y + rectWindow.w - padding, rectWindow.z - padding * 2, 1);
        const auto sampleWidth = 2;
        const auto sampleHeight = height - fontSize - padding * 3;
        const auto sampleCountMax = (uint64_t)(rectBar.z / sampleWidth);
        const auto sampleCountMin = glm::min(sampleCountMax, m_timeHistoryHead + 1ull);

        m_timeHistory.Reserve(sampleCountMax);
        m_timeHistory[m_timeHistoryHead % sampleCountMax] = m_framerate.frameMs;

        auto minHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMax);
        auto maxHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMin);
        auto avgHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateAvg);

        for (auto i = 0ull; i < sampleCountMin; ++i)
        {
            minHistoryTime = glm::min(minHistoryTime, m_timeHistory[i]);
            maxHistoryTime = glm::max(maxHistoryTime, m_timeHistory[i]);
        }

        FixedString64 textFramerateCur("FPS: %i", m_framerate.framerate);
        FixedString64 textFramerateAvg("Avg: %4.2fms", avgHistoryTime);
        FixedString64 textFramerateMin("Min: %4.2fms", minHistoryTime);
        FixedString64 textFramerateMax("Max: %4.2fms", maxHistoryTime);

        gui->GUIDrawRect(COLOR_BG, rectWindow);
        gui->GUIDrawWireRect(COLOR_FG, rectWindow, 1);
        gui->GUIDrawText(COLOR_FPS_AVG, short4(rectWindow.xy + short2(padding * 2 + 100 * 0, padding + 2), 0, 0), textFramerateCur.c_str(), FontStyle().SetSize(16.0f));
        gui->GUIDrawText(COLOR_FPS_AVG, short4(rectWindow.xy + short2(padding * 2 + 100 * 1, padding + 2), 0, 0), textFramerateAvg.c_str(), FontStyle().SetSize(16.0f));
        gui->GUIDrawText(COLOR_FPS_MIN, short4(rectWindow.xy + short2(padding * 2 + 100 * 2, padding + 2), 0, 0), textFramerateMin.c_str(), FontStyle().SetSize(16.0f));
        gui->GUIDrawText(COLOR_FPS_MAX, short4(rectWindow.xy + short2(padding * 2 + 100 * 3, padding + 2), 0, 0), textFramerateMax.c_str(), FontStyle().SetSize(16.0f));

        for (auto i = 0ull; i < sampleCountMin; ++i)
        {
            const auto s_offset = (i + sampleCountMax - sampleCountMin) * sampleWidth;
            const auto s_sample = m_timeHistory[(m_timeHistoryHead + i + 1ull) % sampleCountMin];
            const auto s_normalized = (s_sample - minHistoryTime) / (maxHistoryTime - minHistoryTime);
            const auto s_height = (int)glm::round(sampleHeight * s_normalized);
            const auto s_color = Math::HueToRGB32((1.0f - s_normalized) / 3.0f);
            gui->GUIDrawRect(color32(s_color.r, s_color.g, s_color.b, 127), rectSample + short4(s_offset, 0, 0, -s_height));
        }

        gui->GUIDrawRect(COLOR_FPS_MIN, rectBar + short4(0, -sampleHeight * 0, 0, 0));
        gui->GUIDrawRect(COLOR_FPS_AVG, rectBar + short4(0, -sampleHeight / 2, 0, 0));
        gui->GUIDrawRect(COLOR_FPS_MAX, rectBar + short4(0, -sampleHeight * 1, 0, 0));

        m_timeHistoryHead++;
    }
}
