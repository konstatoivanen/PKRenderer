#include "PrecompiledHeader.h"
#include "Core/Utilities/Bitmask.h"
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
    
    void EngineProfiler::AnalyzeShaderResourceLayouts()
    {
        /*
        auto shaders = m_assetDatabase->GetAssetsOfType<ShaderAsset>();

        std::unordered_set<uint32_t> uniqueNames;
        std::unordered_map<uint32_t, Bitmask<1024>> usagemasks;

        for (const auto& shader : shaders)
        {
            for (auto i = 0u; i < shader->GetRHICount(); ++i)
            {
                auto rhi = shader->GetRHI(i);

                for (auto j = 0u; j < PK_RHI_MAX_DESCRIPTOR_SETS; ++j)
                {
                    auto set = rhi->GetResourceLayout(j);

                    for (auto k = 0u; k < set.GetCount(); ++k)
                    {
                        uniqueNames.insert(set[k].name);
                    }
                }
            }
        }

        usagemasks.reserve(shaders.size());

        for (const auto& shader : shaders)
        {


            for (auto i = 0u; i < shader->GetRHICount(); ++i)
            {
                auto rhi = shader->GetRHI(i);

                for (auto j = 0u; j < PK_RHI_MAX_DESCRIPTOR_SETS; ++j)
                {
                    auto set = rhi->GetResourceLayout(j);

                    for (auto k = 0u; k < set.GetCount(); ++k)
                    {
                        uniqueNames.insert(set[k].name);
                    }
                }
            }
        }

        PK_LOG_INFO("Listing shader resource usage counters:");
        PK_LOG_SCOPE_INDENT();

        for (auto& kv : namedCounters)
        {
            NameID name = kv.first;
            PK_LOG_INFO("%s : %u", name.c_str(), kv.second);
        }
        */
    }

    void EngineProfiler::Step(IGUIRenderer* gui)
    {
        // @TODO this is pretty hacky & hard coded. fix later
        auto renderArea = gui->GUIGetRenderAreaRect();
        auto boxRect = short4(4, 78, renderArea.z - 8, -74);

        auto barRect = short4(boxRect.x + 8, boxRect.y + boxRect.w + 7, 2, 1);
        auto barSpacing = 2;
        auto barHeightRange = 38;
        auto barCount = (boxRect.z - 16) / barSpacing;

        m_timeHistoryHead %= barCount;
        m_timeHistory.resize(barCount);
        m_timeHistory.at(m_timeHistoryHead) = m_framerate.frameMs;

        auto minHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMax);
        auto maxHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMin);
        auto avgHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateAvg);

        for (auto i = 0; i < barCount; ++i)
        {
            minHistoryTime = glm::min(minHistoryTime, m_timeHistory.at(i));
            maxHistoryTime = glm::max(maxHistoryTime, m_timeHistory.at(i));
        }

        FixedString64 textFramerate("FPS: %i", m_framerate.framerate);
        FixedString64 textFramerateAvg("Avg: %4.2fms", avgHistoryTime);
        FixedString64 textFramerateMin("Min: %4.2fms", minHistoryTime);
        FixedString64 textFramerateMax("Max: %4.2fms", maxHistoryTime);

        gui->GUIDrawRect(color32(0,0,0,192), boxRect);
        gui->GUIDrawWireRect(color32(255, 255, 255, 64), boxRect, 1);
        gui->GUIDrawText(color32(255,255,255,127), boxRect.xy + short2(12, -3), textFramerate.c_str(), 16.0f);
        gui->GUIDrawText(color32(255,255,255,127), boxRect.xy + short2(12 + 72, -3), textFramerateAvg.c_str(), 16.0f);
        gui->GUIDrawText(color32(  0,255,  0,127), boxRect.xy + short2(12 + 72 + 100  * 1, -3), textFramerateMin.c_str(), 16.0f);
        gui->GUIDrawText(color32(255,  0,  0,127), boxRect.xy + short2(12 + 72 + 100 * 2, -3), textFramerateMax.c_str(), 16.0f);

        gui->GUIDrawRect(color32(0, 255, 0, 127), short4(boxRect.x + 8, barRect.y, boxRect.z - 16, 1));
        gui->GUIDrawRect(color32(127, 127, 127, 127), short4(boxRect.x + 8, barRect.y + barHeightRange / 2, boxRect.z - 16, 1));
        gui->GUIDrawRect(color32(255, 0, 0, 127), short4(boxRect.x + 8, barRect.y + barHeightRange, boxRect.z - 16, 1));

        for (auto i = 0; i < barCount; ++i)
        {
            auto time = m_timeHistory.at((m_timeHistoryHead + i + 1) % barCount);
            auto interp = (time - minHistoryTime) / (maxHistoryTime - minHistoryTime);
            auto height = (int)glm::round(barHeightRange * interp);
            auto color = Math::HueToRGB32((1.0f - interp) / 3.0f);
            gui->GUIDrawRect(color32(color.r, color.g, color.b, 127), short4(barRect.x + barSpacing * i, barRect.y, barRect.z, height));
        }

        m_timeHistoryHead++;
        m_timeHistoryHead %= barCount;   
    }
}
