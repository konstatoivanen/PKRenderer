#include "PrecompiledHeader.h"
#include "Core/Platform/PlatformInterfaces.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Math/Color.h"
#include "Core/Math/Rect.h"
#include "Core/Math/Extended.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Font.h"
#include "Core/GUI/GUIWidgets.h"
#include "EngineProfiler.h"

namespace PK::App
{
    EngineProfiler::EngineProfiler() : m_window(GUIWindowStyle::GetRedDark())
    {
        m_window.style.align = PK_FLOAT2_RIGHT;
        m_window.style.minSize = short2(256, 128);
        m_window.style.maxSize = short2(1024, 256);
        m_window.style.initialSize = short2(256, 256);
        m_window.style.contentMode = GUILayoutMode::Flow;

        CVariableRegister::Create<CVariableFuncSimple>("Engine.Profiler.Toggle", [this]() { m_enabled ^= true; });
    }
    
    void EngineProfiler::Step(GUI* gui)
    {
        if (!m_enabled)
        {
            return;
        }

        const auto cpumemory = Platform::GetMemoryInfo();
        const auto gpumemory = RHI::GetMemoryInfo();

        m_samples[m_sampleHead % MAX_SAMPLE_COUNT] = m_framerate.frameMs;
        m_sampleHead++;

        const auto sampleCount = (uint32_t)math::min(m_sampleHead, (uint64_t)MAX_SAMPLE_COUNT);
        const auto avgHistoryTime = math::max(1e-4, 1000.0 / m_framerate.framerateAvg);
        
        auto minHistoryTime = math::max(1e-4, 1000.0 / m_framerate.framerateMax);
        auto maxHistoryTime = math::max(1e-4, 1000.0 / m_framerate.framerateMin);

        for (auto i = 0ull; i < MAX_SAMPLE_COUNT; ++i)
        {
            minHistoryTime = math::min(minHistoryTime, m_samples[i]);
            maxHistoryTime = math::max(maxHistoryTime, m_samples[i]);
        }

        GUIWindow::Begin(gui, &m_window, "PROFILER");
        {
            GUILabel::Fit(gui, FixedString64("FPS: %03u", m_framerate.framerate), m_window.style.label);
            GUILabel::Fit(gui, FixedString64("AVG: %4.2fms", avgHistoryTime), m_window.style.label);
            GUILabel::Fit(gui, FixedString64("MIN: %4.2fms", minHistoryTime), m_window.style.label);
            GUILabel::Fit(gui, FixedString64("MAX: %4.2fms", maxHistoryTime), m_window.style.label);
            GUILabel::Fit(gui, FixedString64("RAM.P: %s", String::FormatBytes<16>(cpumemory.programMemoryUsedExclusive).c_str()), m_window.style.label);
            GUILabel::Fit(gui, FixedString64("RAM.T: %s", String::FormatBytes<16>(cpumemory.programMemoryUsedInclusive).c_str()), m_window.style.label);
            GUILabel::Fit(gui, FixedString64("VRAM: %s", String::FormatBytes<16>(gpumemory.usedBytes).c_str()), m_window.style.label);

            gui->BeginLayout({ .mode = GUILayoutMode::Partition }, PK_SHORT4_ZERO);

            if (gui->GetLayoutArea().w > 32 && sampleCount > 2)
            {
                gui->NextLayoutRect({ 0,2,0,0 });
                gui->DrawRect(m_window.style.label.colorFg, gui->NextLayoutRect({ 0,2,0,0 }));
                gui->DrawRect(m_window.style.label.colorFg, gui->NextLayoutRect({ 0,0,0,2 }));              
                
                const auto vertexSpacingPreferred = 8u;
                
                const auto area_graph = gui->NextLayoutRect();
                const auto vertexCount = math::min(area_graph.z / vertexSpacingPreferred, sampleCount);
                const auto vertexSpacing = area_graph.z / vertexCount;
                const auto vertexSampleCount = math::max(1u, 1u << math::log2(sampleCount / vertexCount));
                const auto vertexBinCount = MAX_SAMPLE_COUNT / vertexSampleCount;
                
                int16_t offsets_avg[MAX_SAMPLE_COUNT]{};
                int16_t offsets_min[MAX_SAMPLE_COUNT]{};
                int16_t offsets_max[MAX_SAMPLE_COUNT]{};

                for (auto i = 0ull; i < vertexBinCount; ++i)
                {
                    auto min = 1.0f;
                    auto max = 0.0f;
                    auto avg = 0.0f;

                    for (auto j = 0u; j < vertexSampleCount; ++j)
                    {
                        const auto s_sample = m_samples[i * vertexSampleCount + j];
                        const auto s_normalized = (float)((s_sample - minHistoryTime) / (maxHistoryTime - minHistoryTime));
                        avg = avg + s_normalized;
                        min = math::min(min, s_normalized);
                        max = math::max(max, s_normalized);
                    }

                    avg /= vertexSampleCount;
                    offsets_avg[i] = (1.0f - avg) * area_graph.w;
                    offsets_min[i] = (1.0f - min) * area_graph.w;
                    offsets_max[i] = (1.0f - max) * area_graph.w;
                }

                for (auto i = 0ull; i < vertexCount; ++i)
                {
                    const auto index_high = m_sampleHead - 1ull;
                    const auto index_low = index_high / vertexSampleCount;
                    const auto index_bin = (index_low - i) % vertexBinCount;
                    const auto index_frac = index_high % vertexSampleCount;
                    const auto sub_offset = index_frac * vertexSpacing / vertexSampleCount;

                    {
                        auto rect = PK_SHORT4_ZERO;
                        rect.x = area_graph.x + i * vertexSpacing + sub_offset;
                        rect.z = vertexSpacing / 3;
                        rect.y = area_graph.y + offsets_avg[index_bin];
                        rect.w = offsets_min[index_bin] - offsets_avg[index_bin];
                        gui->DrawRect(m_window.style.field.colorHoverFg, rect);
                    }

                    {
                        auto rect = PK_SHORT4_ZERO;
                        rect.x = area_graph.x + i * vertexSpacing + sub_offset;
                        rect.z = vertexSpacing / 3;
                        rect.y = area_graph.y + offsets_avg[index_bin];
                        rect.w = offsets_max[index_bin] - offsets_avg[index_bin];
                        gui->DrawRect(m_window.style.field.colorFg, rect);
                    }
                }

            }
            
            gui->EndLayout();
        }
        GUIWindow::End(gui, &m_window);

        if (m_window.requestsClose)
        {
            m_enabled = false;
        }
    }
}
