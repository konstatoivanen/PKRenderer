#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Utilities/FixedString.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "Core/Rendering/TextureAsset.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/EntityCulling.h"
#include "App/Renderer/RenderView.h"
#include "RenderPipelineBase.h"

namespace PK::App
{
    RenderPipelineBase::RenderPipelineBase(EntityDatabase* entityDb, AssetDatabase* assetDatabase, Sequencer* sequencer, IBatcher* batcher) :
        m_sequencer(sequencer),
        m_entityDb(entityDb),
        m_batcher(batcher),
        m_renderViewCount(0u)
    {
        auto hash = HashCache::Get();

        auto bluenoise256 = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_Bluenoise256.ktx2")->GetRHI();
        auto bluenoise128x64 = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_Bluenoise128x64.ktx2")->GetRHI();

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

        RHI::SetTexture(hash->pk_Bluenoise256, bluenoise256);
        RHI::SetTexture(hash->pk_Bluenoise128x64, bluenoise128x64);

        SamplerDescriptor samplerDesc{};
        samplerDesc.anisotropy = 16.0f;
        samplerDesc.filterMin = FilterMode::Trilinear;
        samplerDesc.filterMag = FilterMode::Trilinear;
        samplerDesc.wrap[0] = WrapMode::Repeat;
        samplerDesc.wrap[1] = WrapMode::Repeat;
        samplerDesc.wrap[2] = WrapMode::Repeat;
        RHI::SetSampler(hash->pk_Sampler_SurfDefault, samplerDesc);

        samplerDesc.anisotropy = 0.0f;
        samplerDesc.filterMin = FilterMode::Bilinear;
        samplerDesc.filterMag = FilterMode::Bilinear;
        samplerDesc.wrap[0] = WrapMode::Clamp;
        samplerDesc.wrap[1] = WrapMode::Clamp;
        samplerDesc.wrap[2] = WrapMode::Clamp;
        RHI::SetSampler(hash->pk_Sampler_GBuffer, samplerDesc);
    }

    void RenderPipelineBase::OnApplicationRender(RHIWindow* window)
    {
        auto entityViews = m_entityDb->Query<EntityViewRenderView>((uint32_t)ENTITY_GROUPS::ACTIVE);

        PK_WARNING_ASSERT(entityViews.count < MAX_RENDER_VIEWS, "Active scene view count exceeds predefined maximum (%i)", MAX_RENDER_VIEWS);

        m_renderViewCount = 0u;
        RenderView* views[MAX_RENDER_VIEWS]{};

        for (auto i = 0u; i < entityViews.count; ++i)
        {
            auto& entity = entityViews[i];

            uint4 viewrect = entity.renderView->desiredRect;

            if (entity.renderView->isWindowTarget)
            {
                viewrect.z = glm::max(0, (int)glm::min(viewrect.x + viewrect.z, window->GetResolution().x) - (int)viewrect.x);
                viewrect.w = glm::max(0, (int)glm::min(viewrect.y + viewrect.w, window->GetResolution().y) - (int)viewrect.y);
            }

            if (viewrect.z > 0 && viewrect.w > 0 && m_renderViewCount < MAX_RENDER_VIEWS)
            {
                views[m_renderViewCount] = &m_renderViews[m_renderViewCount];
                auto renderView = &m_renderViews[m_renderViewCount++];
                auto isViewChanged = renderView->viewEntityId != entity.GID.entityID();
                auto viewresolution = viewrect.zw - viewrect.xy;
                auto bufferResolution = GBuffers::AlignResolution(viewresolution);
                auto bufferAspectRatio = (float)bufferResolution.x / (float)bufferResolution.y;
                auto isOutOfDate = bufferResolution != renderView->bufferResolution;

                entity.renderView->renderViewRef = renderView;
                renderView->viewEntityId = entity.GID.entityID();
                renderView->type = entity.renderView->type;
                renderView->primaryPassGroup = 0u;
                renderView->isWindowTarget = entity.renderView->isWindowTarget;
                renderView->settings = *entity.renderView->settingsRef;
                renderView->renderAreaRect = viewrect;
                renderView->renderAreaRect.x += (bufferResolution.x - viewresolution.x) / 2;
                renderView->renderAreaRect.y += (bufferResolution.y - viewresolution.y) / 2;
                renderView->bufferResolution = bufferResolution;
                renderView->finalViewRect = viewrect;
                renderView->timeRender = m_timeFrameInfo;
                renderView->viewToClip = entity.projection->ResolveProjectionMatrix(bufferAspectRatio);
                renderView->worldToView = entity.transform->worldToLocal;
                renderView->worldToClip = renderView->viewToClip * renderView->worldToView;
                renderView->forwardPlane = Math::TransformPlane(entity.transform->localToWorld, float4(0, 0, 1, 0));
                renderView->znear = Math::GetZNearFromClip(renderView->viewToClip);
                renderView->zfar = Math::GetZFarFromClip(renderView->viewToClip);

                if (isOutOfDate || isViewChanged)
                {
                    renderView->timeResize = m_timeFrameInfo;
                }
            }
        }

        if (m_renderViewCount == 0)
        {
            return;
        }

        auto cullingProxy = EntityCullSequencerProxy(m_sequencer, this);

        RenderPipelineContext context;
        context.sequencer = m_sequencer;
        context.entityDb = m_entityDb;
        context.cullingProxy = &cullingProxy;
        context.batcher = m_batcher;
        context.window = window;
        context.views = views;
        context.viewCount = m_renderViewCount;

        Render(&context);

        auto* cmdgraphics = RHI::GetCommandBuffer(QueueType::Graphics);

        // @TODO composite multiple views to an intermediate window target

        for (auto i = 0u; i < m_renderViewCount; ++i)
        {
            auto view = &m_renderViews[i];

            if (view->isWindowTarget && view->gbuffers.current.color)
            {
                cmdgraphics->Blit(view->gbuffers.current.color.get(), window, FilterMode::Bilinear);
            }
        }
    }

    void RenderPipelineBase::ValidateViewGBuffers(RenderView* view, const GBuffersFullDescriptor& descriptors)
    {
        if (view->gbuffers.Validate(view->bufferResolution, descriptors, "RenderView"))
        {
            view->timeResize = m_timeFrameInfo;
        }
    }
    
    void RenderPipelineBase::ValidateViewConstantBuffer(RenderView* view, const BufferLayout& layout)
    {
        if (view->constants == nullptr || !view->constants->GetLayout().CompareFast(layout))
        {
            view->constants = CreateRef<ConstantBuffer>(layout, "RenderView.Constants.Frame");
            view->timeResize = m_timeFrameInfo;
        }
    }

    void RenderPipelineBase::DispatchRenderPipelineEvent(RHICommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type)
    {
        // all active views should be of the same type
        auto view = context->views[0];

        if (type != RenderPipelineEvent::CollectDraws)
        {
            cmd->BeginDebugScope(FixedString64({RenderViewTypeName[(int)view->type], RenderPipelineEvent::TypeNames[type]}).c_str(), PK_COLOR_GREEN);
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
