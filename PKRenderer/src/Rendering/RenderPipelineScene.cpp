#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsMatrix.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/HashCache.h"
#include "Rendering/RequestRayTracingGeometry.h"
#include "Rendering/Objects/RenderView.h"
#include "Rendering/RHI/Driver.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Objects/QueueSet.h"
#include "Rendering/RHI/Window.h"
#include "RenderPipelineScene.h"

namespace PK::Rendering
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Core::ControlFlow;
    using namespace PK::Core::Services;
    using namespace PK::ECS;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Geometry;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    RenderPipelineScene::RenderPipelineScene(EntityDatabase* entityDb, AssetDatabase* assetDatabase, ApplicationConfig* config) :
        m_passHierarchicalDepth(assetDatabase, config),
        m_passEnvBackground(assetDatabase),
        m_passPostEffectsComposite(assetDatabase, config),
        m_passLights(assetDatabase, config),
        m_passSceneGI(assetDatabase, config),
        m_passVolumeFog(assetDatabase, config),
        m_passFilmGrain(assetDatabase),
        m_depthOfField(assetDatabase, config),
        m_temporalAntialiasing(assetDatabase, config->InitialWidth, config->InitialHeight),
        m_bloom(assetDatabase, config->InitialWidth, config->InitialHeight),
        m_autoExposure(assetDatabase)
    {
        PK_LOG_VERBOSE("SceneRenderPipeline.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_sceneStructure = AccelerationStructure::Create("Scene");

        auto hash = HashCache::Get();

        AssetImportEvent<ApplicationConfig> token{ assetDatabase, config };
        Step(&token);

        m_constantsLayout = BufferLayout(
            {
                { ElementType::Float3x4, hash->pk_WorldToView },
                { ElementType::Float3x4, hash->pk_ViewToWorld },
                { ElementType::Float3x4, hash->pk_ViewToWorldPrev },

                { ElementType::Float4x4, hash->pk_ViewToClip },
                { ElementType::Float4x4, hash->pk_WorldToClip },
                { ElementType::Float4x4, hash->pk_WorldToClip_NoJitter },
                { ElementType::Float4x4, hash->pk_WorldToClipPrev },
                { ElementType::Float4x4, hash->pk_WorldToClipPrev_NoJitter },
                { ElementType::Float4x4, hash->pk_ViewToClipDelta },

                { ElementType::Float4, hash->pk_Time },
                { ElementType::Float4, hash->pk_SinTime },
                { ElementType::Float4, hash->pk_CosTime },
                { ElementType::Float4, hash->pk_DeltaTime },
                { ElementType::Float4, hash->pk_CursorParams },
                { ElementType::Float4, hash->pk_ViewWorldOrigin },
                { ElementType::Float4, hash->pk_ViewWorldOriginPrev },
                { ElementType::Float4, hash->pk_ViewSpaceCameraDelta },
                { ElementType::Float4, hash->pk_ClipParams },
                { ElementType::Float4, hash->pk_ClipParamsInv },
                { ElementType::Float4, hash->pk_ClipParamsExp },
                { ElementType::Float4, hash->pk_ScreenParams },
                { ElementType::Float4, hash->pk_ShadowCascadeZSplits },
                { ElementType::Float4, hash->pk_ProjectionJitter },
                { ElementType::Uint4, hash->pk_FrameRandom },
                { ElementType::Uint2, hash->pk_ScreenSize },
                { ElementType::Uint2, hash->pk_FrameIndex },
                { ElementType::Float, hash->pk_SceneEnv_Exposure }
            });
    }

    RenderPipelineScene::~RenderPipelineScene()
    {
        GraphicsAPI::GetDriver()->WaitForIdle();
        m_sceneStructure = nullptr;
    }

    GBuffersFull::Descriptor RenderPipelineScene::GetViewGBufferDescriptors() const
    {
        GBuffersFull::Descriptor desc;
        desc.current.usages[GBuffers::Color] = TextureUsage::RTColorSample | TextureUsage::Storage;
        desc.current.usages[GBuffers::Normals] = TextureUsage::RTColorSample;
        desc.current.usages[GBuffers::DepthBiased] = TextureUsage::RTColorSample;
        desc.current.usages[GBuffers::Depth] = TextureUsage::RTDepthSample;
        desc.current.formats[GBuffers::Color] = TextureFormat::RGBA16F;
        desc.current.formats[GBuffers::Normals] = TextureFormat::RGB10A2;
        desc.current.formats[GBuffers::DepthBiased] = TextureFormat::R32F;
        desc.current.formats[GBuffers::Depth] = TextureFormat::Depth32F;

        desc.previous.usages[GBuffers::Color] = TextureUsage::Default | TextureUsage::Storage;
        desc.previous.usages[GBuffers::Normals] = TextureUsage::Default;
        desc.previous.usages[GBuffers::DepthBiased] = TextureUsage::Default;
        desc.previous.usages[GBuffers::Depth] = TextureUsage::Default;
        // @TODO refactor to use RGB9E5 as rgba16f redundantly big.
        desc.previous.formats[GBuffers::Color] = TextureFormat::RGBA16F;
        desc.previous.formats[GBuffers::Normals] = TextureFormat::RGB10A2;
        desc.previous.formats[GBuffers::DepthBiased] = TextureFormat::R32F;
        desc.previous.formats[GBuffers::Depth] = TextureFormat::Depth32F;
        return desc;
    }

    void RenderPipelineScene::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        
        auto& constants = view->constants;
        auto resolution = view->GetResolution();
        auto isOutOfDate = view->timeRender.frameIndex == view->timeResize.frameIndex;

        const auto shadowCascadeZSplits = m_passLights.GetCascadeZSplitsFloat4(view->znear, view->zfar);
        const auto projectionJitter = m_temporalAntialiasing.GetJitter();
        const auto time = view->timeRender.time;
        const auto deltaTime = view->timeRender.deltaTime;
        const auto smoothDeltaTime = view->timeRender.smoothDeltaTime;
        const auto frameIndex = view->timeRender.frameIndex;
        const auto frameIndexResize = view->timeResize.frameIndex;

        float2 jitter =
        {
            projectionJitter.x / resolution.x,
            projectionJitter.y / resolution.y
        };

        const auto viewToClipNoJitter = view->viewToClip;
        const auto viewToClip = Functions::GetPerspectiveJittered(viewToClipNoJitter, jitter);
        const auto clipToView = glm::inverse(viewToClip);
        const auto worldToView = view->worldToView;
        const auto viewToWorld = glm::inverse(worldToView);
        const auto worldToClip = viewToClip * worldToView;
        const auto worldToClipNoJitter = viewToClipNoJitter * worldToView;
        const auto n = view->znear;
        const auto f = view->zfar;

        auto viewToWorldPrev = Functions::TransposeTo3x4(viewToWorld);
        auto worldToClipPrev = worldToClip;
        auto worldToClipPrevNoJitter = worldToClipNoJitter;

        constants->TryGet(hash->pk_ViewToWorld, viewToWorldPrev);
        constants->TryGet(hash->pk_WorldToClip, worldToClipPrev);
        constants->TryGet(hash->pk_WorldToClip_NoJitter, worldToClipPrevNoJitter);

        auto viewSpaceCameraDelta = worldToView * float4(viewToWorldPrev[0].w, viewToWorldPrev[1].w, viewToWorldPrev[2].w, 1.0f);

        constants->Set<float3x4>(hash->pk_WorldToView, Functions::TransposeTo3x4(worldToView));
        constants->Set<float3x4>(hash->pk_ViewToWorld, Functions::TransposeTo3x4(viewToWorld));
        constants->Set<float3x4>(hash->pk_ViewToWorldPrev, viewToWorldPrev);

        constants->Set<float4x4>(hash->pk_ViewToClip, viewToClip);
        constants->Set<float4x4>(hash->pk_WorldToClip, worldToClip);
        constants->Set<float4x4>(hash->pk_WorldToClip_NoJitter, worldToClipNoJitter);
        constants->Set<float4x4>(hash->pk_WorldToClipPrev, worldToClipPrev);
        constants->Set<float4x4>(hash->pk_WorldToClipPrev_NoJitter, worldToClipPrevNoJitter);
        constants->Set<float4x4>(hash->pk_ViewToClipDelta, worldToClipPrev * viewToWorld);

        constants->Set<float4>(hash->pk_Time, { (float)time / 20.0f, (float)time, (float)time * 2.0f, (float)time * 3.0f });
        constants->Set<float4>(hash->pk_SinTime, { (float)sin(time / 8.0f), (float)sin(time / 4.0f), (float)sin(time / 2.0f), (float)sin(time) });
        constants->Set<float4>(hash->pk_CosTime, { (float)cos(time / 8.0f), (float)cos(time / 4.0f), (float)cos(time / 2.0f), (float)cos(time) });
        constants->Set<float4>(hash->pk_DeltaTime, { (float)deltaTime, 1.0f / (float)deltaTime, (float)smoothDeltaTime, 1.0f / (float)smoothDeltaTime });
        constants->Set<float4>(hash->pk_CursorParams, PK_FLOAT4_ZERO); // @TODO
        constants->Set<float4>(hash->pk_ViewWorldOrigin, viewToWorld[3]);
        constants->Set<float4>(hash->pk_ViewWorldOriginPrev, float4(viewToWorldPrev[0].w, viewToWorldPrev[1].w, viewToWorldPrev[2].w, 1.0f));
        constants->Set<float4>(hash->pk_ViewSpaceCameraDelta, viewSpaceCameraDelta);
        constants->Set<float4>(hash->pk_ClipParams, { n, f, viewToClip[2][2], viewToClip[3][2] });
        constants->Set<float4>(hash->pk_ClipParamsInv, { clipToView[0][0], clipToView[1][1], clipToView[2][3], clipToView[3][3] });
        constants->Set<float4>(hash->pk_ClipParamsExp, { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n });
        constants->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        constants->Set<float4>(hash->pk_ProjectionJitter, projectionJitter);
        constants->Set<uint4>(hash->pk_FrameRandom, Functions::MurmurHash41((uint32_t)(frameIndex % ~0u)));
        constants->Set<uint2>(hash->pk_ScreenSize, { resolution.x, resolution.y });
        constants->Set<uint2>(hash->pk_FrameIndex, { frameIndex % 0xFFFFFFFFu, (frameIndex - frameIndexResize) % 0xFFFFFFFFu }); 
        constants->Set<float4>(hash->pk_ShadowCascadeZSplits, shadowCascadeZSplits);
        constants->Set<float>(hash->pk_SceneEnv_Exposure, m_backgroundExposure);
    }

    void RenderPipelineScene::RenderViews(RenderPipelineContext* context)
    {
        // @TODO add multi view support
        auto view = context->views[0];

        auto hash = HashCache::Get();
        auto queues = GraphicsAPI::GetQueues();
        auto* cmdtransfer = queues->GetCommandBuffer(QueueType::Transfer);
        auto* cmdgraphics = queues->GetCommandBuffer(QueueType::Graphics);
        auto* cmdcompute = queues->GetCommandBuffer(QueueType::Compute);

        auto gbuffers = view->GetGBuffersFullView();
        auto resolution = view->GetResolution();
        GraphicsAPI::SetTexture(hash->pk_GB_Current_Normals, gbuffers.current.normals);
        GraphicsAPI::SetTexture(hash->pk_GB_Current_Depth, gbuffers.current.depth);
        GraphicsAPI::SetTexture(hash->pk_GB_Current_DepthBiased, gbuffers.current.depthBiased);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_Color, gbuffers.previous.color);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_Normals, gbuffers.previous.normals);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_Depth, gbuffers.previous.depth);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_DepthBiased, gbuffers.previous.depthBiased);
        GraphicsAPI::SetBuffer(hash->pk_PerFrameConstants, *view->constants.get());
        
        m_passSceneGI.PreRender(cmdtransfer, resolution);
        context->batcher->BeginCollectDrawCalls();
        {
            DispatchRenderPipelineEvent(cmdtransfer, context, RenderPipelineEvent::CollectDraws);
            m_passLights.BuildLights(context);
        }
        context->batcher->EndCollectDrawCalls(cmdtransfer);

        // Prune voxels & build AS.
        // These can happen before the end of last frame. 
        m_passSceneGI.PruneVoxels(cmdcompute);
        context->sequencer->NextEmplace<RequestRayTracingGeometry>
        (
            this, QueueType::Compute, m_sceneStructure.get(), ScenePrimitiveFlags::DefaultMesh, BoundingBox(), false
        );
        GraphicsAPI::SetAccelerationStructure(hash->pk_SceneStructure, m_sceneStructure.get());
        queues->Submit(QueueType::Compute, &cmdcompute);

        // End transfer operations
        queues->Sync(QueueType::Graphics, QueueType::Transfer, -1);
        queues->Submit(QueueType::Transfer);
        // Sync previous frames graphics accesses to compute queue (image layouts etc.)
        queues->Transfer(QueueType::Graphics, QueueType::Compute);
        queues->Sync(QueueType::Transfer, QueueType::Graphics);
        queues->Sync(QueueType::Transfer, QueueType::Compute);

        // Only buffering needs to wait for previous results.
        // Eliminate redundant rendering waits by waiting for transfer instead.
        context->window->SetFrameFence(queues->GetFenceRef(QueueType::Transfer));

        // Depth pre pass. Meshlet cull based on prev frame hizb
        cmdgraphics->SetRenderTarget({ gbuffers.current.depth }, true);
        cmdgraphics->ClearDepth(PK_CLIPZ_FAR, 0u);
        cmdgraphics->SetStageExcludeMask(ShaderStageFlags::Fragment);
        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::Depth);
        cmdgraphics->SetStageExcludeMask(ShaderStageFlags::None);

        // Gbuffer pass & possible depth writes from invalid z culls.
        cmdgraphics->SetRenderTarget({ gbuffers.current.depth, gbuffers.current.normals, gbuffers.current.depthBiased }, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 1);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 2);

        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::GBuffer);
        m_passHierarchicalDepth.Compute(cmdgraphics, resolution);
        queues->Submit(QueueType::Graphics, &cmdgraphics);

        // Async compute during last present.
        m_passFilmGrain.Compute(cmdcompute);
        m_passLights.ComputeClusters(cmdcompute, context);
        m_autoExposure.Render(cmdcompute, m_bloom.GetTexture());
        m_depthOfField.ComputeAutoFocus(cmdcompute, resolution.y);
        m_passVolumeFog.ComputeDensity(cmdcompute, resolution);
        m_passEnvBackground.ComputeSH(cmdcompute);
        m_passSceneGI.VoxelMips(cmdcompute);
        m_passSceneGI.ValidateReservoirs(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        queues->Sync(QueueType::Graphics, QueueType::Compute);

        // Indirect GI ray tracing
        m_passSceneGI.DispatchRays(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        // Wait for misc async compute instead of ray dispatch
        queues->Sync(QueueType::Compute, QueueType::Graphics, -1);

        // Shadows, Voxelize scene & reproject gi
        m_passLights.RenderShadows(cmdgraphics, context);
        m_passSceneGI.Voxelize(cmdgraphics, context->batcher, view->primaryPassGroup);
        m_passLights.RenderScreenSpaceShadows(cmdgraphics, context);
        m_passSceneGI.ReprojectGI(cmdgraphics);
        m_passVolumeFog.Compute(cmdgraphics, gbuffers.current.color->GetResolution());
        queues->Submit(QueueType::Graphics, &cmdgraphics);
        queues->Transfer(QueueType::Graphics, QueueType::Compute);
        queues->Wait(QueueType::Compute, QueueType::Graphics);

        m_passSceneGI.RenderGI(cmdgraphics);

        // Forward Opaque on graphics queue
        cmdgraphics->SetRenderTarget({ gbuffers.current.depth, gbuffers.current.color }, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::ForwardOpaque);

        m_passEnvBackground.RenderBackground(cmdgraphics);

        m_passVolumeFog.Render(cmdgraphics, gbuffers.current.color);

        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::ForwardTransparent);

        // Cache forward output of current frame
        cmdgraphics->Blit(gbuffers.current.color, gbuffers.previous.color, {}, {}, FilterMode::Point);

        // Post Effects
        cmdgraphics->BeginDebugScope("PostEffects", PK_COLOR_YELLOW);
        {
            // Previous color has been updated. leverage that and do taa without extra blit.
            m_temporalAntialiasing.Render(cmdgraphics, gbuffers.previous.color, gbuffers.current.color);
            m_depthOfField.Render(cmdgraphics, gbuffers.current.color);
            m_bloom.Render(cmdgraphics, gbuffers.current.color);
            m_passPostEffectsComposite.Render(cmdgraphics, gbuffers.current.color);
        }
        cmdgraphics->EndDebugScope();

        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::AfterPostEffects);

        queues->Submit(QueueType::Graphics, &cmdgraphics);

        cmdgraphics->Blit(gbuffers.current.normals, gbuffers.previous.normals, {}, {}, FilterMode::Point);
        cmdgraphics->Blit(gbuffers.current.depthBiased, gbuffers.previous.depthBiased, {}, {}, FilterMode::Point);
        cmdgraphics->Blit(gbuffers.current.depth, gbuffers.previous.depth, {}, {}, FilterMode::Point);
    }

    void RenderPipelineScene::Step(AssetImportEvent<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto config = token->asset;
        m_backgroundExposure = config->BackgroundExposure;
        m_passPostEffectsComposite.OnUpdateParameters(token);
        m_passEnvBackground.OnUpdateParameters(token);
        m_passSceneGI.OnUpdateParameters(config);
        m_depthOfField.OnUpdateParameters(config);
        m_passVolumeFog.OnUpdateParameters(config);
    }
}