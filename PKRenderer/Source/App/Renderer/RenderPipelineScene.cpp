#include "PrecompiledHeader.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/Math/FunctionsMatrix.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/EntityCulling.h"
#include "App/Renderer/RenderView.h"
#include "RenderPipelineScene.h"

namespace PK::App
{
    RenderPipelineScene::RenderPipelineScene(AssetDatabase* assetDatabase,
        EntityDatabase* entityDb,
        Sequencer* sequencer,
        IBatcher* batcher,
        const uint2& initialResolution) : 
        RenderPipelineBase(entityDb, assetDatabase, sequencer, batcher),
        m_passLights(assetDatabase, initialResolution),
        m_passSceneGI(assetDatabase, initialResolution),
        m_passVolumeFog(assetDatabase, initialResolution),
        m_passHierarchicalDepth(assetDatabase, initialResolution),
        m_passEnvBackground(assetDatabase),
        m_passFilmGrain(assetDatabase),
        m_depthOfField(assetDatabase, initialResolution),
        m_temporalAntialiasing(assetDatabase, initialResolution),
        m_bloom(assetDatabase, initialResolution),
        m_autoExposure(assetDatabase),
        m_passPostEffectsComposite(assetDatabase)
    {
        PK_LOG_VERBOSE("SceneRenderPipeline.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_sceneStructure = RHI::CreateAccelerationStructure("Scene");
        
        auto hash = HashCache::Get();

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
            { ElementType::Float, hash->pk_SceneEnv_Exposure },

            // GI Parameters
            { ElementType::Float, hash->pk_GI_VoxelSize },
            { ElementType::Float, hash->pk_GI_VoxelStepSize },
            { ElementType::Float, hash->pk_GI_VoxelLevelScale },
            { ElementType::Float4, hash->pk_GI_VolumeST },
            { ElementType::Uint4, hash->pk_GI_VolumeSwizzle },
            { ElementType::Uint2, hash->pk_GI_RayDither },

            // Fog Parameters
            { ElementType::Float,  hash->pk_Fog_Density_Amount },
            { ElementType::Float,  hash->pk_Fog_Density_Constant },
            { ElementType::Float4, hash->pk_Fog_Albedo },
            { ElementType::Float4, hash->pk_Fog_Absorption },
            { ElementType::Float4, hash->pk_Fog_WindDirSpeed },

            { ElementType::Float,  hash->pk_Fog_Phase0 },
            { ElementType::Float,  hash->pk_Fog_Phase1 },
            { ElementType::Float,  hash->pk_Fog_PhaseW },
            { ElementType::Float,  hash->pk_Fog_Density_HeightExponent },
            
            { ElementType::Float,  hash->pk_Fog_Density_HeightOffset },
            { ElementType::Float,  hash->pk_Fog_Density_HeightAmount },
            { ElementType::Float,  hash->pk_Fog_Density_NoiseAmount },
            { ElementType::Float,  hash->pk_Fog_Density_NoiseScale },
            
            { ElementType::Float,  hash->pk_Fog_Density_Sky_Constant },
            { ElementType::Float,  hash->pk_Fog_Density_Sky_HeightExponent },
            { ElementType::Float,  hash->pk_Fog_Density_Sky_HeightOffset },
            { ElementType::Float,  hash->pk_Fog_Density_Sky_HeightAmount },

            // Color Grading
            { ElementType::Float4, hash->pk_CC_WhiteBalance },
            { ElementType::Float4, hash->pk_CC_Lift },
            { ElementType::Float4, hash->pk_CC_Gamma },
            { ElementType::Float4, hash->pk_CC_Gain },
            { ElementType::Float4, hash->pk_CC_HSV },
            { ElementType::Float4, hash->pk_CC_MixRed },
            { ElementType::Float4, hash->pk_CC_MixGreen },
            { ElementType::Float4, hash->pk_CC_MixBlue },

            { ElementType::Float, hash->pk_CC_LumaContrast },
            { ElementType::Float, hash->pk_CC_LumaGain },
            { ElementType::Float, hash->pk_CC_LumaGamma },
            { ElementType::Float, hash->pk_CC_Vibrance },
            { ElementType::Float, hash->pk_CC_Contribution },

            // Vignette 
            { ElementType::Float, hash->pk_Vignette_Intensity },
            { ElementType::Float, hash->pk_Vignette_Power },

            // Film grain
            { ElementType::Float, hash->pk_FilmGrain_Luminance },
            { ElementType::Float, hash->pk_FilmGrain_Intensity },

            // Auto exposure
            { ElementType::Float, hash->pk_AutoExposure_MinLogLuma },
            { ElementType::Float, hash->pk_AutoExposure_InvLogLumaRange },
            { ElementType::Float, hash->pk_AutoExposure_LogLumaRange },
            { ElementType::Float, hash->pk_AutoExposure_Target },
            { ElementType::Float, hash->pk_AutoExposure_Speed },

            // Bloom
            { ElementType::Float, hash->pk_Bloom_Intensity },
            { ElementType::Float, hash->pk_Bloom_DirtIntensity },

            // Temporal anti aliasing
            { ElementType::Float, hash->pk_TAA_Sharpness },
            { ElementType::Float, hash->pk_TAA_BlendingStatic },
            { ElementType::Float, hash->pk_TAA_BlendingMotion },
            { ElementType::Float, hash->pk_TAA_MotionAmplification },

            { ElementType::Uint, hash->pk_PostEffectsFeatureMask}
        });
    }

    RenderPipelineScene::~RenderPipelineScene()
    {
        RHI::GetDriver()->WaitForIdle();
        m_sceneStructure = nullptr;
    }

    void RenderPipelineScene::Render(RenderPipelineContext* context)
    {
        auto hash = HashCache::Get();
        auto queues = RHI::GetQueues();
        auto* cmdtransfer = queues->GetCommandBuffer(QueueType::Transfer);
        
        for (auto i = 0u; i < context->viewCount; ++i)
        {
            auto view = context->views[i];
        
            GBuffersFullDescriptor desc;
            desc.current[GBuffers::Color] = { TextureFormat::RGBA16F, TextureUsage::RTColorSample | TextureUsage::Storage };
            desc.current[GBuffers::Normals] = { TextureFormat::RGB10A2, TextureUsage::RTColorSample };
            desc.current[GBuffers::DepthBiased] = { TextureFormat::R32F, TextureUsage::RTColorSample };
            desc.current[GBuffers::Depth] = { TextureFormat::Depth32F, TextureUsage::RTDepthSample };

            // @TODO refactor to use RGB9E5 as rgba16f redundantly big.
            desc.previous[GBuffers::Color] = { TextureFormat::RGBA16F, TextureUsage::Default | TextureUsage::Storage };
            desc.previous[GBuffers::Normals] = { TextureFormat::RGB10A2, TextureUsage::Default };
            desc.previous[GBuffers::DepthBiased] = { TextureFormat::R32F, TextureUsage::Default };
            desc.previous[GBuffers::Depth] = { TextureFormat::Depth32F, TextureUsage::Default };
            
            ValidateViewGBuffers(view, desc);
            ValidateViewConstantBuffer(view, m_constantsLayout);

            auto constants = view->constants.get();
            auto resolution = view->GetResolution();

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
            const auto viewToClip = Math::GetPerspectiveJittered(viewToClipNoJitter, jitter);
            const auto clipToView = glm::inverse(viewToClip);
            const auto worldToView = view->worldToView;
            const auto viewToWorld = glm::affineInverse(worldToView);
            const auto worldToClip = viewToClip * worldToView;
            const auto worldToClipNoJitter = viewToClipNoJitter * worldToView;
            const auto n = view->znear;
            const auto f = view->zfar;
            auto viewToWorldPrev = Math::TransposeTo3x4(viewToWorld);
            auto worldToClipPrev = worldToClip;
            auto worldToClipPrevNoJitter = worldToClipNoJitter;

            constants->TryGet(hash->pk_ViewToWorld, viewToWorldPrev);
            constants->TryGet(hash->pk_WorldToClip, worldToClipPrev);
            constants->TryGet(hash->pk_WorldToClip_NoJitter, worldToClipPrevNoJitter);

            auto viewSpaceCameraDelta = worldToView * float4(viewToWorldPrev[0].w, viewToWorldPrev[1].w, viewToWorldPrev[2].w, 1.0f);

            constants->Set<float3x4>(hash->pk_WorldToView, Math::TransposeTo3x4(worldToView));
            constants->Set<float3x4>(hash->pk_ViewToWorld, Math::TransposeTo3x4(viewToWorld));
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
            constants->Set<uint4>(hash->pk_FrameRandom, Math::MurmurHash41((uint32_t)(frameIndex % ~0u)));
            constants->Set<uint2>(hash->pk_ScreenSize, { resolution.x, resolution.y });
            constants->Set<uint2>(hash->pk_FrameIndex, { frameIndex % 0xFFFFFFFFu, (frameIndex - frameIndexResize) % 0xFFFFFFFFu });
            constants->Set<float4>(hash->pk_ShadowCascadeZSplits, shadowCascadeZSplits);

            m_passEnvBackground.SetViewConstants(view);
            m_passSceneGI.SetViewConstants(view);
            m_passVolumeFog.SetViewConstants(view);
            m_bloom.SetViewConstants(view);
            m_autoExposure.SetViewConstants(view);
            m_depthOfField.SetViewConstants(view);
            m_passFilmGrain.SetViewConstants(view);
            m_temporalAntialiasing.SetViewConstants(view);
            m_passPostEffectsComposite.SetViewConstants(view);
            constants->FlushBuffer(cmdtransfer);
        }

        // @TODO add multi view support
        auto primaryView = context->views[0];
        auto gbuffers = primaryView->GetGBuffersFullView();
        auto resolution = primaryView->GetResolution();
        RHI::SetTexture(hash->pk_GB_Current_Normals, gbuffers.current.normals);
        RHI::SetTexture(hash->pk_GB_Current_Depth, gbuffers.current.depth);
        RHI::SetTexture(hash->pk_GB_Current_DepthBiased, gbuffers.current.depthBiased);
        RHI::SetTexture(hash->pk_GB_Previous_Color, gbuffers.previous.color);
        RHI::SetTexture(hash->pk_GB_Previous_Normals, gbuffers.previous.normals);
        RHI::SetTexture(hash->pk_GB_Previous_Depth, gbuffers.previous.depth);
        RHI::SetTexture(hash->pk_GB_Previous_DepthBiased, gbuffers.previous.depthBiased);
        RHI::SetBuffer(hash->pk_PerFrameConstants, *primaryView->constants.get());

        m_passSceneGI.PreRender(cmdtransfer, resolution);
        context->batcher->BeginCollectDrawCalls();
        {
            DispatchRenderPipelineEvent(cmdtransfer, context, RenderPipelineEvent::CollectDraws);
            m_passLights.BuildLights(context);
        }
        context->batcher->EndCollectDrawCalls(cmdtransfer);

        // Prune voxels & build AS.
        // These can happen before the end of last frame. 
        auto* cmdcompute = queues->GetCommandBuffer(QueueType::Compute);
        m_passSceneGI.PruneVoxels(cmdcompute);
        context->cullingProxy->CullRayTracingGeometry(ScenePrimitiveFlags::DefaultMesh, BoundingBox(), false, QueueType::Compute, m_sceneStructure.get());
        RHI::SetAccelerationStructure(hash->pk_SceneStructure, m_sceneStructure.get());
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
        CommandBufferExt cmdgraphics = queues->GetCommandBuffer(QueueType::Graphics);
        cmdgraphics.SetRenderTarget({ gbuffers.current.depth }, true);
        cmdgraphics->ClearDepth(PK_CLIPZ_FAR, 0u);
        cmdgraphics->SetStageExcludeMask(ShaderStageFlags::Fragment);
        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::Depth);
        cmdgraphics->SetStageExcludeMask(ShaderStageFlags::None);

        // Gbuffer pass & possible depth writes from invalid z culls.
        cmdgraphics.SetRenderTarget({ gbuffers.current.depth, gbuffers.current.normals, gbuffers.current.depthBiased }, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 1);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 2);

        DispatchRenderPipelineEvent(cmdgraphics, context, RenderPipelineEvent::GBuffer);
        m_passHierarchicalDepth.Compute(cmdgraphics, resolution);
        cmdgraphics = queues->Submit(QueueType::Graphics);

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
        m_passSceneGI.Voxelize(cmdgraphics, context->batcher, primaryView->primaryPassGroup);
        m_passLights.RenderScreenSpaceShadows(cmdgraphics, context);
        m_passSceneGI.ReprojectGI(cmdgraphics);
        m_passVolumeFog.Compute(cmdgraphics, gbuffers.current.color->GetResolution());
        cmdgraphics = queues->Submit(QueueType::Graphics);
        queues->Transfer(QueueType::Graphics, QueueType::Compute);
        queues->Wait(QueueType::Compute, QueueType::Graphics);

        m_passSceneGI.RenderGI(cmdgraphics);

        // Forward Opaque on graphics queue
        cmdgraphics.SetRenderTarget({ gbuffers.current.depth, gbuffers.current.color }, true);
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

        cmdgraphics = queues->Submit(QueueType::Graphics);
        cmdgraphics->Blit(gbuffers.current.normals, gbuffers.previous.normals, {}, {}, FilterMode::Point);
        cmdgraphics->Blit(gbuffers.current.depthBiased, gbuffers.previous.depthBiased, {}, {}, FilterMode::Point);
        cmdgraphics->Blit(gbuffers.current.depth, gbuffers.previous.depth, {}, {}, FilterMode::Point);
    }
}