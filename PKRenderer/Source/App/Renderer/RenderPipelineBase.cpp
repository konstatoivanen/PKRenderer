#include "PrecompiledHeader.h"
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Containers/FixedArena.h"
#include "Core/Math/Projection.h"
#include "Core/Math/Bounds.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Window.h"
#include "App/ECS/EntityViewRenderView.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/EntityCulling.h"
#include "App/Renderer/RenderView.h"
#include "App/FrameContext.h"
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

        {
            auto bluenoise256 = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_Bluenoise256.pktexture")->GetRHI();
            auto bluenoise128x64 = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_Bluenoise128x64.pktexture")->GetRHI();
            RHI::SetTexture(hash->pk_Bluenoise256, bluenoise256);
            RHI::SetTexture(hash->pk_Bluenoise128x64, bluenoise128x64);
        }

        // Pre integrate DFG texture for ibl shading.
        {
            TextureDescriptor descr{};
            descr.type = TextureType::Texture2D;
            descr.format = TextureFormat::RGBA16_Float;
            descr.resolution = { 128u, 128u, 1u };
            descr.usage = TextureUsage::Sample | TextureUsage::Storage;
            m_integratedDFG = RHI::CreateTexture(descr, "PKBuiltIn.Texture2D.PreintegratedDFG");
            auto integrateDFGShader = assetDatabase->Find<ShaderAsset>("CS_IntegrateDFG").get();

            RHI::SetImage(hash->pk_Image, m_integratedDFG.get());
            RHI::SetTexture(hash->pk_PreIntegratedDFG, m_integratedDFG.get());
            RenderGraphExt(RHI::GetGraph()).Dispatch(QueueType::Graphics, integrateDFGShader, { 128, 128, 1 });
        }
    }

    void RenderPipelineBase::OnStepFrameRender(FrameContext* ctx)
    {
        auto window = ctx->window;
        auto entityViews = m_entityDb->Query<EntityViewRenderView>();

        PK_WARNING_ASSERT(entityViews.count() < MAX_RENDER_VIEWS, "Active scene view count exceeds predefined maximum (%i)", MAX_RENDER_VIEWS);

        m_renderViewCount = 0u;
        RenderView* views[MAX_RENDER_VIEWS]{};

        for (auto& entityView : entityViews)
        {
            auto viewrect = entityView.renderView->desiredRect;

            if (entityView.renderView->isWindowTarget)
            {
                viewrect.z = math::max(0, (int32_t)math::min(viewrect.x + viewrect.z, window->GetResolution().x) - (int32_t)viewrect.x);
                viewrect.w = math::max(0, (int32_t)math::min(viewrect.y + viewrect.w, window->GetResolution().y) - (int32_t)viewrect.y);
            }

            if (viewrect.z > 0 && viewrect.w > 0 && m_renderViewCount < MAX_RENDER_VIEWS)
            {
                views[m_renderViewCount] = &m_renderViews[m_renderViewCount];
                auto resources = GetViewResourceSet(m_renderViewCount);
                auto renderView = &m_renderViews[m_renderViewCount++];
                auto hasViewChanged = renderView->viewEntityId != *entityView.entityId;
                auto viewresolution = viewrect.zw - viewrect.xy;
                auto bufferResolution = GBuffers::AlignResolution(viewresolution);
                auto bufferAspectRatio = (float)bufferResolution.x / (float)bufferResolution.y;
                auto isOutOfDate = math::any(bufferResolution != renderView->bufferResolution);

                entityView.renderView->renderViewRef = renderView;
                renderView->resources = resources;
                renderView->viewEntityId = *entityView.entityId;
                renderView->name = entityView.renderView->name;
                renderView->primaryPassGroup = 0u;
                renderView->isWindowTarget = entityView.renderView->isWindowTarget;
                renderView->settings = *entityView.renderView->settingsRef;
                renderView->renderAreaRect = viewrect;
                renderView->renderAreaRect.x += (bufferResolution.x - viewresolution.x) / 2;
                renderView->renderAreaRect.y += (bufferResolution.y - viewresolution.y) / 2;
                renderView->bufferResolution = bufferResolution;
                renderView->finalViewRect = viewrect;
                renderView->timeRender = entityView.time->info;
                renderView->worldToViewPrev = renderView->worldToView;
                renderView->viewToClipPrev = renderView->viewToClip;
                renderView->viewToClip = entityView.projection->ResolveProjectionMatrix(bufferAspectRatio);
                renderView->fieldOfView = entityView.projection->fieldOfView * PK_FLOAT_DEG2RAD;
                renderView->worldToView = entityView.transform->worldToLocal;
                renderView->worldToClip = renderView->viewToClip * renderView->worldToView;
                renderView->forwardPlane = math::mulplanar(entityView.transform->localToWorld, float4(0, 0, 1, 0));
                renderView->position = entityView.transform->position;
                renderView->znear = math::nearClip(renderView->viewToClip);
                renderView->zfar = math::farClip(renderView->viewToClip);

                renderView->cursorPosition = entityView.input->state.cursorPosition;
                renderView->cursorPositionDelta = entityView.input->state.cursorPositionDelta;

                if (hasViewChanged)
                {
                    renderView->worldToViewPrev = renderView->worldToView;
                    renderView->viewToClipPrev = renderView->viewToClip;
                }

                if (isOutOfDate || hasViewChanged)
                {
                    renderView->timeResize = entityView.time->info;
                }
            }
        }

        if (m_renderViewCount == 0)
        {
            return;
        }

        auto cullingProxy = EntityCullSequencerProxy(ctx->frameArena, m_sequencer, this);

        RenderPipelineContext context;
        context.frameArena = ctx->frameArena;
        context.sequencer = m_sequencer;
        context.entityDb = m_entityDb;
        context.cullingProxy = &cullingProxy;
        context.batcher = m_batcher;
        context.window = window;
        context.views = views;
        context.viewCount = m_renderViewCount;

        Render(&context);

        // @TODO composite multiple views to an intermediate window target
        for (auto i = 0u; i < m_renderViewCount; ++i)
        {
            auto view = &m_renderViews[i];

            if (view->isWindowTarget && view->gbuffers.current.color)
            {
                RHI::GetGraph()->Blit(QueueType::Graphics, view->gbuffers.current.color.get(), window->GetSwapchain(), FilterMode::Bilinear);
            }
        }
    }

    void RenderPipelineBase::ValidateViewGBuffers(RenderView* view, const GBuffersFullDescriptor& descriptors)
    {
        FixedString32 name({ "RenderView.", view->name });

        if (view->gbuffers.Validate(view->bufferResolution, descriptors, name))
        {
            view->timeResize = view->timeRender;
        }
    }
    
    void RenderPipelineBase::BeginWriteViewConstantBuffer(RenderView* view, IArena* arena, const ShaderPropertyLayout& layout)
    {
        const auto previousHash = view->constants.GetHash();
        FixedString64 name({ "RenderView.", view->name, ".Constants.Frame"});
        view->constants.BeginWrite(&layout, arena->Allocate<uint8_t>(layout.GetStridePadded()), name.c_str());

        if (view->constants.GetHash() != previousHash)
        {
            view->timeResize = view->timeRender;
        }
    }

    void RenderPipelineBase::DispatchRenderPipelineEvent(RHIGraph* graph, RenderPipelineContext* context, RenderPipelineEvent::Type type)
    {
        // all active views should be of the same type
        auto view = context->views[0];

        if (type != RenderPipelineEvent::CollectDraws)
        {
            FixedString64 name({ "RenderViewEvt.", view->name, ".", String::To(type)});
            graph->BeginDebugScope(name, PK_COLOR_GREEN);
        }

        RenderPipelineEvent event;
        event.type = type;
        event.graph = graph;
        event.context = context;

        context->sequencer->Next<RenderPipelineEvent*>(this, &event);

        if (type != RenderPipelineEvent::CollectDraws)
        {
            graph->EndDebugScope();
        }
    }
}
