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

        auto bluenoise256 = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_Bluenoise256.pktexture")->GetRHI();
        auto bluenoise128x64 = assetDatabase->Load<TextureAsset>("Content/Textures/Default/T_Bluenoise128x64.pktexture")->GetRHI();

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

        // Pre integrate DFG texture for ibl shading.
        {
            TextureDescriptor descr{};
            descr.type = TextureType::Texture2D;
            descr.format = TextureFormat::RGBA16F;
            descr.sampler.filterMin = FilterMode::Bilinear;
            descr.sampler.filterMag = FilterMode::Bilinear;
            descr.sampler.wrap[0] = WrapMode::Clamp;
            descr.sampler.wrap[1] = WrapMode::Clamp;
            descr.sampler.wrap[2] = WrapMode::Clamp;
            descr.sampler.borderColor = BorderColor::FloatClear;
            descr.resolution = { 128u, 128u, 1u };
            descr.usage = TextureUsage::Sample | TextureUsage::Storage;
            m_integratedDFG = RHI::CreateTexture(descr, "PKBuiltIn.Texture2D.PreintegratedDFG");
            auto integrateDFGShader = assetDatabase->Find<ShaderAsset>("CS_IntegrateDFG").get();

            RHI::SetImage(hash->pk_Image, m_integratedDFG.get());
            RHI::SetTexture(hash->pk_PreIntegratedDFG, m_integratedDFG.get());
            CommandBufferExt(RHI::GetCommandBuffer(QueueType::Graphics)).Dispatch(integrateDFGShader, { 128, 128, 1 });
            RHI::GetQueues()->Submit(QueueType::Graphics);
        }
    }

    void RenderPipelineBase::OnStepFrameRender(FrameContext* ctx)
    {
        auto window = ctx->window;
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
                auto resources = GetViewResources(m_renderViewCount);
                auto renderView = &m_renderViews[m_renderViewCount++];
                auto isViewChanged = renderView->viewEntityId != entity.GID.entityID();
                auto viewresolution = viewrect.zw - viewrect.xy;
                auto bufferResolution = GBuffers::AlignResolution(viewresolution);
                auto bufferAspectRatio = (float)bufferResolution.x / (float)bufferResolution.y;
                auto isOutOfDate = bufferResolution != renderView->bufferResolution;

                entity.renderView->renderViewRef = renderView;
                renderView->resources = resources;
                renderView->viewEntityId = entity.GID.entityID();
                renderView->name = entity.renderView->name;
                renderView->primaryPassGroup = 0u;
                renderView->isWindowTarget = entity.renderView->isWindowTarget;
                renderView->settings = *entity.renderView->settingsRef;
                renderView->renderAreaRect = viewrect;
                renderView->renderAreaRect.x += (bufferResolution.x - viewresolution.x) / 2;
                renderView->renderAreaRect.y += (bufferResolution.y - viewresolution.y) / 2;
                renderView->bufferResolution = bufferResolution;
                renderView->finalViewRect = viewrect;
                renderView->timeRender = entity.time->info;
                renderView->viewToClip = entity.projection->ResolveProjectionMatrix(bufferAspectRatio);
                renderView->fieldOfView = entity.projection->fieldOfView * PK_FLOAT_DEG2RAD;
                renderView->worldToView = entity.transform->worldToLocal;
                renderView->worldToClip = renderView->viewToClip * renderView->worldToView;
                renderView->forwardPlane = Math::TransformPlane(entity.transform->localToWorld, float4(0, 0, 1, 0));
                renderView->position = entity.transform->position;
                renderView->znear = Math::GetZNearFromClip(renderView->viewToClip);
                renderView->zfar = Math::GetZFarFromClip(renderView->viewToClip);

                renderView->cursorPosition = entity.input->state.cursorPosition;
                renderView->cursorPositionDelta = entity.input->state.cursorPositionDelta;

                if (isOutOfDate || isViewChanged)
                {
                    renderView->timeResize = entity.time->info;
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

        auto* cmdgraphics = RHI::GetCommandBuffer(QueueType::Graphics);

        // @TODO composite multiple views to an intermediate window target

        for (auto i = 0u; i < m_renderViewCount; ++i)
        {
            auto view = &m_renderViews[i];

            if (view->isWindowTarget && view->gbuffers.current.color)
            {
                cmdgraphics->Blit(view->gbuffers.current.color.get(), window->GetSwapchain(), FilterMode::Bilinear);
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
    
    void RenderPipelineBase::ValidateViewConstantBuffer(RenderView* view, const BufferLayout& layout)
    {
        if (view->constants == nullptr || !view->constants->GetLayout().CompareFast(layout))
        {
            FixedString64 name({ "RenderView.", view->name, ".Constants.Frame"});
            view->constants = CreateRef<ConstantBuffer>(layout, name);
            view->timeResize = view->timeRender;
        }
    }

    void RenderPipelineBase::DispatchRenderPipelineEvent(RHICommandBuffer* cmd, RenderPipelineContext* context, RenderPipelineEvent::Type type)
    {
        // all active views should be of the same type
        auto view = context->views[0];

        if (type != RenderPipelineEvent::CollectDraws)
        {
            FixedString64 name({ "RenderViewEvt.", view->name, ".", RenderPipelineEvent::TypeNames[type]});
            cmd->BeginDebugScope(name, PK_COLOR_GREEN);
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
