#include "PrecompiledHeader.h"
#include "Core/Utilities/Bitmask.h"
#include "Core/Utilities/Parsing.h"
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
        auto renderArea = gui->GetRenderAreaRect();
        auto boxRect = short4(4, 76, 16 + 96 * 4, -72);

        auto barRect = short4(boxRect.x + 8, boxRect.y + boxRect.w + 4, 2, 1);
        auto barSpacing = 2;
        auto barHeightRange = 32;
        auto barCount = (boxRect.z - 16) / barSpacing;

        m_timeHistory.resize(barCount);
        m_timeHistory.at(m_timeHistoryHead) = m_framerate.frameMs;

        auto minHistoryTime = glm::max(0.0, 1000.0 / m_framerate.framerateMax);
        auto maxHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateMin);
        auto avgHistoryTime = glm::max(1e-4, 1000.0 / m_framerate.framerateAvg);

        for (auto i = 0; i < barCount; ++i)
        {
            minHistoryTime = glm::min(minHistoryTime, m_timeHistory.at(i));
            maxHistoryTime = glm::max(maxHistoryTime, m_timeHistory.at(i));
        }

        auto textFramerate = Parse::FormatToString("FPS:   %i", m_framerate.framerate);
        auto textFramerateAvg = Parse::FormatToString("Avg: %4.2fms", avgHistoryTime);
        auto textFramerateMin = Parse::FormatToString("Min: %4.2fms", minHistoryTime);
        auto textFramerateMax = Parse::FormatToString("Max: %4.2fms", maxHistoryTime);

        gui->DrawRect(color32(0,0,0,127), boxRect);
        gui->DrawText(color32(255,255,255,127), boxRect.xy + short2(12, -3), textFramerate.c_str(), 16.0f);
        gui->DrawText(color32(255,255,255,127), boxRect.xy + short2(12 + 96 * 1, -3), textFramerateAvg.c_str(), 16.0f);
        gui->DrawText(color32(  0,255,  0,127), boxRect.xy + short2(12 + 96 * 2, -3), textFramerateMin.c_str(), 16.0f);
        gui->DrawText(color32(255,  0,  0,127), boxRect.xy + short2(12 + 96 * 3, -3), textFramerateMax.c_str(), 16.0f);
    
        gui->DrawRect(color32(0, 255, 0, 127), short4(boxRect.x + 8, boxRect.y + boxRect.w + 4, boxRect.z - 16, 1));
        gui->DrawRect(color32(255, 0, 0, 127), short4(boxRect.x + 8, boxRect.y - 36, boxRect.z - 16, 1));

        for (auto i = 0; i < barCount; ++i)
        {
            auto time = m_timeHistory.at((m_timeHistoryHead + i + 1) % barCount);
            auto height = glm::round(barHeightRange * (time - minHistoryTime) / (maxHistoryTime - minHistoryTime));
            gui->DrawRect(color32(255, 255, 255, 64), short4(barRect.x + barSpacing * i, barRect.y, barRect.z, height));
        }

        m_timeHistoryHead++;
        m_timeHistoryHead %= barCount;
        
    }

}