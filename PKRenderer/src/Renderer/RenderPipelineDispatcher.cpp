#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsIntersect.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/Assets/AssetDatabase.h"
#include "ECS/EntityViewRenderView.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/RHI/RHIWindow.h"
#include "Graphics/ConstantBuffer.h"
#include "Graphics/TextureAsset.h"
#include "Renderer/HashCache.h"
#include "Renderer/EntityCulling.h"
#include "Renderer/RenderView.h"
#include "Renderer/IRenderPipeline.h"
#include "RenderPipelineDisptacher.h"

namespace PK::Renderer
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Core::ControlFlow;
    using namespace PK::ECS;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

    RenderPipelineDisptacher::RenderPipelineDisptacher(EntityDatabase* entityDb, AssetDatabase* assetDatabase, Sequencer* sequencer, IBatcher* batcher) :
        m_sequencer(sequencer),
        m_entityDb(entityDb),
        m_batcher(batcher),
        m_renderViewCount(0u)
    {
        auto hash = HashCache::Get();

        auto bluenoise256 = assetDatabase->Load<TextureAsset>("res/textures/default/T_Bluenoise256.ktx2")->GetRHI();
        auto bluenoise128x64 = assetDatabase->Load<TextureAsset>("res/textures/default/T_Bluenoise128x64.ktx2")->GetRHI();

        auto sampler = bluenoise256->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise256->SetSampler(sampler);

        sampler = bluenoise128x64->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise128x64->SetSampler(sampler);

        RHISetTexture(hash->pk_Bluenoise256, bluenoise256);
        RHISetTexture(hash->pk_Bluenoise128x64, bluenoise128x64);

        SamplerDescriptor samplerDesc{};
        samplerDesc.anisotropy = 16.0f;
        samplerDesc.filterMin = FilterMode::Trilinear;
        samplerDesc.filterMag = FilterMode::Trilinear;
        samplerDesc.wrap[0] = WrapMode::Repeat;
        samplerDesc.wrap[1] = WrapMode::Repeat;
        samplerDesc.wrap[2] = WrapMode::Repeat;
        RHISetSampler(hash->pk_Sampler_SurfDefault, samplerDesc);

        samplerDesc.anisotropy = 0.0f;
        samplerDesc.filterMin = FilterMode::Bilinear;
        samplerDesc.filterMag = FilterMode::Bilinear;
        samplerDesc.wrap[0] = WrapMode::Clamp;
        samplerDesc.wrap[1] = WrapMode::Clamp;
        samplerDesc.wrap[2] = WrapMode::Clamp;
        RHISetSampler(hash->pk_Sampler_GBuffer, samplerDesc);
    }

    void RenderPipelineDisptacher::OnApplicationRender(Window* window)
    {
        auto entityViews = m_entityDb->Query<EntityViewRenderView>((uint32_t)ECS::ENTITY_GROUPS::ACTIVE);

        PK_WARNING_ASSERT(entityViews.count < MAX_SCENE_VIEWS, "Active scene view count exceeds predefined maximum (%i)", MAX_SCENE_VIEWS);

        m_renderViewCount = 0u;

        RenderView* viewFamilies[(int)RenderViewType::Count][MAX_SCENE_VIEWS]{};
        uint32_t viewFamilySizes[(int)RenderViewType::Count]{};

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            auto& entity = entityViews[i];
            auto viewTypeIndex = (int)entity.renderView->type;
            auto pipeline = m_renderPipelines[viewTypeIndex];

            if (pipeline != nullptr)
            {
                uint4 viewrect = entity.renderView->desiredRect;

                if (entity.renderView->isWindowTarget)
                {
                    viewrect.z = glm::max(0, (int)glm::min(viewrect.x + viewrect.z, window->GetResolution().x) - (int)viewrect.x);
                    viewrect.w = glm::max(0, (int)glm::min(viewrect.y + viewrect.w, window->GetResolution().y) - (int)viewrect.y);
                }

                if (viewrect.z > 0 && viewrect.w > 0 && m_renderViewCount < MAX_SCENE_VIEWS)
                {
                    auto renderView = &m_renderViews[m_renderViewCount++];

                    auto isViewChanged = renderView->viewEntityId != entity.GID.entityID();

                    renderView->viewEntityId = entity.GID.entityID();
                    renderView->type = entity.renderView->type;
                    renderView->primaryPassGroup = 0u;
                    renderView->isWindowTarget = entity.renderView->isWindowTarget;
                    entity.renderView->renderViewRef = renderView;

                    auto viewresolution = viewrect.zw - viewrect.xy;
                    auto isOutOfDate = renderView->gbuffers.Validate(viewresolution, pipeline->GetViewGBufferDescriptors(), "RenderView");
                    auto resolution = renderView->gbuffers.GetResolution();

                    renderView->renderAreaRect = viewrect;
                    renderView->renderAreaRect.x += (resolution.x - viewresolution.x) / 2;
                    renderView->renderAreaRect.y += (resolution.y - viewresolution.y) / 2;
                    renderView->finalViewRect = viewrect;
                    renderView->timeRender = m_timeFrameInfo;

                    const auto& pipelineConstantsLayout = pipeline->GetViewConstantsLayout();

                    if (renderView->constants == nullptr || !renderView->constants->GetLayout().CompareFast(pipelineConstantsLayout))
                    {
                        renderView->constants = Utilities::CreateRef<ConstantBuffer>(pipelineConstantsLayout, "RenderView.Constants.Frame");
                        isOutOfDate = true;
                    }

                    if (isOutOfDate || isViewChanged)
                    {
                        renderView->timeResize = m_timeFrameInfo;
                    }

                    const auto aspectratio = renderView->gbuffers.GetAspectRatio();
                    renderView->viewToClip = entity.projection->ResolveProjectionMatrix(aspectratio);
                    renderView->worldToView = entity.transform->worldToLocal;
                    renderView->worldToClip = renderView->viewToClip * renderView->worldToView;
                    renderView->forwardPlane = Functions::TransformPlane(entity.transform->localToWorld, float4(0, 0, 1, 0));
                    renderView->znear = Functions::GetZNearFromClip(renderView->viewToClip);
                    renderView->zfar = Functions::GetZFarFromClip(renderView->viewToClip);

                    pipeline->SetViewConstants(renderView);

                    renderView->constants->FlushBuffer(RHIGetCommandBuffer(QueueType::Transfer));

                    viewFamilies[viewTypeIndex][viewFamilySizes[viewTypeIndex]++] = renderView;
                }
            }
        }

        for (auto familyIndex = 0u; familyIndex < (int)RenderViewType::Count; ++familyIndex)
        {
            if (viewFamilySizes[familyIndex] > 0)
            {
                auto cullingProxy = EntityCullSequencerProxy(m_sequencer, m_renderPipelines[familyIndex]);

                RenderPipelineContext context;
                context.sequencer = m_sequencer;
                context.entityDb = m_entityDb;
                context.cullingProxy = &cullingProxy;
                context.batcher = m_batcher;
                context.window = window;
                context.views = viewFamilies[familyIndex];
                context.viewCount = viewFamilySizes[familyIndex];
                m_renderPipelines[familyIndex]->RenderViews(&context);
            }
        }

        auto* cmdgraphics = RHIGetCommandBuffer(QueueType::Graphics);

        // @TODO composite multiple views to an intermediate window target

        for (auto i = 0u; i < m_renderViewCount; ++i)
        {
            auto view = &m_renderViews[i];

            if (view->isWindowTarget)
            {
                cmdgraphics->Blit(view->gbuffers.current.color.get(), window, FilterMode::Bilinear);
            }
        }
    }

    void IRenderPipeline::DispatchRenderPipelineEvent(CommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type)
    {
        // all active views should be of the same type
        auto view = context->views[0];

        if (type != RenderPipelineEvent::CollectDraws)
        {
            cmd->BeginDebugScope((std::string(RenderViewTypeName[(int)view->type]) + RenderPipelineEvent::TypeNames[type]).c_str(), PK_COLOR_GREEN);
        }

        RenderPipelineEvent event;
        event.type = type;
        event.cmd = cmd;
        event.context = context;

        context->sequencer->Next<RenderPipelineEvent*>(this, &event);

        if (type != RenderPipelineEvent::CollectDraws)
        {
            cmd->EndDebugScope();
        }
    }
}
