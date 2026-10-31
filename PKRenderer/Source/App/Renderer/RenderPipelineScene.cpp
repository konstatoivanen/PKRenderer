#include "PrecompiledHeader.h"
#include "Core/Math/Random.h"
#include "Core/Math/Projection.h"
#include "Core/CLI/Log.h"
#include "Core/ControlFlow/FenceRef.h"
#include "Core/ControlFlow/Sequencer.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "Core/Rendering/Window.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/EntityCulling.h"
#include "App/Renderer/RenderView.h"
#include "RenderPipelineScene.h"

namespace PK::App
{
    RenderPipelineScene::RenderPipelineScene(AssetDatabase* assetDatabase,
        EntityDatabase* entityDb,
        Sequencer* sequencer,
        IBatcher* batcher) : 
        IRenderPipeline(entityDb, assetDatabase, sequencer, batcher),
        m_passLights(assetDatabase),
        m_passSceneGI(assetDatabase),
        m_passVolumetricFog(assetDatabase),
        m_passHierarchicalDepth(assetDatabase),
        m_passSceneEnv(assetDatabase),
        m_passFilmGrain(assetDatabase),
        m_depthOfField(assetDatabase),
        m_temporalAntialiasing(assetDatabase),
        m_distort(assetDatabase),
        m_bloom(assetDatabase),
        m_autoExposure(assetDatabase),
        m_passPostEffectsComposite(assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC();

        m_sceneStructure = RHI::CreateAccelerationStructure("Scene");

        auto hash = HashCache::Get();

        m_constantsLayout = ShaderPropertyLayout(
        {
            { Uniform::Float3x4, hash->WorldToView },
            { Uniform::Float3x4, hash->ViewToWorld },
            { Uniform::Float3x4, hash->ViewToWorldPrev },

            { Uniform::Float4x4, hash->ViewToClip },
            { Uniform::Float4x4, hash->WorldToClip },
            { Uniform::Float4x4, hash->WorldToClip_NoJitter },
            { Uniform::Float4x4, hash->WorldToClipPrev },
            { Uniform::Float4x4, hash->WorldToClipPrev_NoJitter },
            { Uniform::Float4x4, hash->ViewToPrevClip },
            { Uniform::Float4x4, hash->ClipToPrevClip_NoJitter },

            { Uniform::Float4, hash->Time },
            { Uniform::Float4, hash->SinTime },
            { Uniform::Float4, hash->CosTime },
            { Uniform::Float4, hash->DeltaTime },
            { Uniform::Float4, hash->CursorParams },
            { Uniform::Float4, hash->ViewWorldOrigin },
            { Uniform::Float4, hash->ViewWorldOriginPrev },
            { Uniform::Float4, hash->ViewSpaceCameraDelta },
            { Uniform::Float4, hash->ClipParams },
            { Uniform::Float4, hash->ClipParamsInv },
            { Uniform::Float4, hash->ScreenParams },
            { Uniform::Float4, hash->ProjectionJitter },
            { Uniform::Uint4, hash->FrameRandom },
            { Uniform::Uint2, hash->ScreenSize },
            { Uniform::Uint2, hash->FrameIndex },

            { Uniform::Float4, hash->MeshletCullParams },
            { Uniform::Float4, hash->ShadowCascadeZSplits },
            { Uniform::Float4, hash->LightTileZParams },

            // GI Parameters
            { Uniform::Float3, hash->GI_VX_TexelSize },
            { Uniform::Float,  hash->GI_VX_StepSize },
            { Uniform::Uint3,  hash->GI_VX_Swizzle },
            { Uniform::Float,  hash->GI_VX_LevelScale },
            { Uniform::Float4, hash->GI_VX_ST },
            { Uniform::Uint2,  hash->GI_RayDither },

            // Fog Parameters
            { Uniform::Float,  hash->Fog_Density_NoiseAmount },
            { Uniform::Float,  hash->Fog_Density_NoiseScale },
            { Uniform::Float,  hash->Fog_Density_Amount },
            { Uniform::Float,  hash->Fog_Phase0 },
            { Uniform::Float,  hash->Fog_Phase1 },
            { Uniform::Float,  hash->Fog_PhaseW },
            { Uniform::Float4, hash->Fog_Albedo },
            { Uniform::Float4, hash->Fog_ZParams },
            { Uniform::Float4, hash->Fog_FadeParams },
            { Uniform::Float4, hash->Fog_Absorption },
            { Uniform::Float4, hash->Fog_WindDirSpeed },
            { Uniform::Float4, hash->Fog_Density_ExpParams0 },
            { Uniform::Float4, hash->Fog_Density_ExpParams1 },

            { Uniform::Float4, hash->Panini_Projection_Parameters },

            // Color Grading
            { Uniform::Float4, hash->CC_WhiteBalance },
            { Uniform::Float4, hash->CC_Lift },
            { Uniform::Float4, hash->CC_Gamma },
            { Uniform::Float4, hash->CC_Gain },
            { Uniform::Float4, hash->CC_HSV },
            { Uniform::Float4, hash->CC_MixRed },
            { Uniform::Float4, hash->CC_MixGreen },
            { Uniform::Float4, hash->CC_MixBlue },

            { Uniform::Float, hash->CC_LumaContrast },
            { Uniform::Float, hash->CC_LumaGain },
            { Uniform::Float, hash->CC_LumaGamma },
            { Uniform::Float, hash->CC_Vibrance },
            { Uniform::Float, hash->CC_Contribution },

            // Vignette 
            { Uniform::Float, hash->Vignette_Intensity },
            { Uniform::Float, hash->Vignette_Power },

            // Film grain
            { Uniform::Float, hash->FilmGrain_Luminance },
            { Uniform::Float, hash->FilmGrain_Intensity },
            { Uniform::Float, hash->FilmGrain_ExposureSensitivity },

            // Auto exposure
            { Uniform::Float, hash->AutoExposure_LogLumaRange },
            { Uniform::Float, hash->AutoExposure_Target },
            { Uniform::Float, hash->AutoExposure_Min },
            { Uniform::Float, hash->AutoExposure_Max },
            { Uniform::Float, hash->AutoExposure_Speed },

            // Bloom
            { Uniform::Float, hash->Bloom_Diffusion },
            { Uniform::Float, hash->Bloom_Intensity },
            { Uniform::Float, hash->Bloom_DirtIntensity },

            // Chromatic aberration
            { Uniform::Float, hash->Chromatic_Aberration_Amount },
            { Uniform::Float, hash->Chromatic_Aberration_Power },

            // Temporal anti aliasing
            { Uniform::Float, hash->TAA_Sharpness },
            { Uniform::Float, hash->TAA_BlendingStatic },
            { Uniform::Float, hash->TAA_BlendingMotion },
            { Uniform::Float, hash->TAA_MotionAmplification },

            { Uniform::Uint, hash->ScreenLevels },
            { Uniform::Uint, hash->PostEffectsFeatureMask}
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
        RenderGraphExt graph = RHI::GetGraph();
        
        for (auto i = 0u; i < context->viewCount; ++i)
        {
            auto view = context->views[i];
        
            GBuffersFullDescriptor desc;
            desc.current[GBuffers::Color] = { TextureFormat::RGBA16_Float, TextureUsage::RTColorSample | TextureUsage::Storage, false };
            desc.current[GBuffers::Normals] = { TextureFormat::RGB10A2_Unorm, TextureUsage::RTColorSample, true };
            desc.current[GBuffers::DepthBiased] = { TextureFormat::R32_Float, TextureUsage::RTColorSample, true };
            desc.current[GBuffers::Depth] = { TextureFormat::Depth32_Float, TextureUsage::RTDepthSample, true };

            desc.previous[GBuffers::Color] = { TextureFormat::RGBA16_Float, TextureUsage::Default | TextureUsage::Storage, false };
            desc.previous[GBuffers::Normals] = { TextureFormat::RGB10A2_Unorm, TextureUsage::RTColorSample, true };
            desc.previous[GBuffers::DepthBiased] = { TextureFormat::R32_Float, TextureUsage::RTColorSample, true };
            desc.previous[GBuffers::Depth] = { TextureFormat::Depth32_Float, TextureUsage::RTDepthSample, true };
            
            ValidateViewGBuffers(view, desc);
            BeginWriteViewConstantBuffer(view, context->frameArena, m_constantsLayout);

            auto constants = &view->constants;
            auto resolution = view->GetResolution();

            const auto projectionJitter = m_temporalAntialiasing.GetJitter();
            const auto time = view->timeRender.time;
            const auto deltaTime = view->timeRender.deltaTime;
            const auto smoothDeltaTime = view->timeRender.smoothDeltaTime;
            const auto frameIndex = view->timeRender.frameIndex;
            const auto frameIndexResize = view->timeResize.frameIndex;

            float4 jitter =
            {
                projectionJitter.x / resolution.x,
                projectionJitter.y / resolution.y,
                projectionJitter.z / resolution.x,
                projectionJitter.w / resolution.y
            };

            const auto& viewToClipNoJitter = view->viewToClip;
            const auto& worldToView = view->worldToView;
            const auto viewToClip = math::perspectiveJittered(viewToClipNoJitter, jitter.xy());
            const auto clipToView = math::inverse(viewToClip);
            const auto viewToWorld = math::affineInverse(worldToView);
            const auto worldToClip = viewToClip * worldToView;
            const auto worldToClipNoJitter = viewToClipNoJitter * worldToView;
            const auto n = view->znear;
            const auto f = view->zfar;
            const auto aspect = (resolution.x / (float)resolution.y);

            const auto& viewToClipNoJitterPrev = view->viewToClipPrev;
            const auto& worldToViewPrev = view->worldToViewPrev;
            const auto viewToClipPrev = math::perspectiveJittered(viewToClipNoJitterPrev, jitter.xy());
            const auto viewToWorldPrev = math::affineInverse(worldToViewPrev);
            const auto worldToClipPrev = viewToClipPrev * worldToViewPrev;
            const auto worldToClipNoJitterPrev = viewToClipNoJitterPrev * worldToViewPrev;

            auto viewSpaceCameraDelta = worldToView * float4(viewToWorldPrev[3].xyz, 1.0f);

            constants->Set<float3x4>(hash->WorldToView, math::transpose3x4(worldToView));
            constants->Set<float3x4>(hash->ViewToWorld, math::transpose3x4(viewToWorld));
            constants->Set<float3x4>(hash->ViewToWorldPrev, math::transpose3x4(viewToWorldPrev));
            constants->Set<float4x4>(hash->ViewToClip, viewToClip);
            constants->Set<float4x4>(hash->WorldToClip, worldToClip);
            constants->Set<float4x4>(hash->WorldToClip_NoJitter, worldToClipNoJitter);
            constants->Set<float4x4>(hash->WorldToClipPrev, worldToClipPrev);
            constants->Set<float4x4>(hash->WorldToClipPrev_NoJitter, worldToClipNoJitterPrev);
            constants->Set<float4x4>(hash->ViewToPrevClip, worldToClipPrev * viewToWorld);
            constants->Set<float4x4>(hash->ClipToPrevClip_NoJitter, worldToClipNoJitterPrev * math::inverse(worldToClipNoJitter));
            constants->Set<float4>(hash->Time, { time / 20.0, time, time * 2.0, time * 3.0 });
            constants->Set<float4>(hash->SinTime, { math::sin(time / 8.0), math::sin(time / 4.0), math::sin(time / 2.0), math::sin(time) });
            constants->Set<float4>(hash->CosTime, { math::cos(time / 8.0), math::cos(time / 4.0), math::cos(time / 2.0), math::cos(time) });
            constants->Set<float4>(hash->DeltaTime, { deltaTime, 1.0 / deltaTime, smoothDeltaTime, 1.0 / smoothDeltaTime });
            constants->Set<float4>(hash->CursorParams, { view->cursorPosition.x, view->cursorPosition.y, view->cursorPositionDelta.x, view->cursorPositionDelta.y });
            constants->Set<float4>(hash->ViewWorldOrigin, viewToWorld[3]);
            constants->Set<float4>(hash->ViewWorldOriginPrev, float4(viewToWorldPrev[0].w, viewToWorldPrev[1].w, viewToWorldPrev[2].w, 1.0f));
            constants->Set<float4>(hash->ViewSpaceCameraDelta, viewSpaceCameraDelta);
            constants->Set<float4>(hash->ClipParams, { n, f, viewToClip[2][2], viewToClip[3][2] });
            constants->Set<float4>(hash->ClipParamsInv, { clipToView[0][0], clipToView[1][1], clipToView[2][3], clipToView[3][3] });
            constants->Set<float4>(hash->ScreenParams, { resolution.x, resolution.y, 1.0f / resolution.x, 1.0f / resolution.y });
            constants->Set<float4>(hash->ProjectionJitter, projectionJitter);
            constants->Set<uint4>(hash->FrameRandom, math::murmurhash41((uint32_t)(frameIndex % ~0u)));
            constants->Set<uint2>(hash->ScreenSize, { resolution.x, resolution.y });
            constants->Set<uint2>(hash->FrameIndex, { frameIndex % 0xFFFFFFFFu, (frameIndex - frameIndexResize) % 0xFFFFFFFFu });
            constants->Set<int>(hash->ScreenLevels, math::levels(resolution.xy()) - 1u);
            constants->Set<float4>(hash->MeshletCullParams, { 1.0f / (viewToClip[1][1] * resolution.y * 0.5f), view->fieldOfView * aspect, view->fieldOfView, 1.0f });

            m_passHierarchicalDepth.SetViewConstants(view);
            m_passLights.SetViewConstants(view);
            m_passSceneEnv.SetViewConstants(view);
            m_passSceneGI.SetViewConstants(view);
            m_passVolumetricFog.SetViewConstants(view);
            m_distort.SetViewConstants(view);
            m_bloom.SetViewConstants(view);
            m_autoExposure.SetViewConstants(view);
            m_depthOfField.SetViewConstants(view);
            m_passFilmGrain.SetViewConstants(view);
            m_temporalAntialiasing.SetViewConstants(view);
            m_passPostEffectsComposite.SetViewConstants(view);
            constants->EndWrite(graph, QueueType::Transfer);
        }

        // @TODO add multi view support
        auto primaryView = context->views[0];
        auto gbuffers = primaryView->GetGBuffersFullView();
        RHI::SetTexture(hash->pk_GB_Current_Normals, gbuffers.current.normals);
        RHI::SetTexture(hash->pk_GB_Current_Depth, gbuffers.current.depth);
        RHI::SetTexture(hash->pk_GB_Current_DepthBiased, gbuffers.current.depthBiased);
        RHI::SetTexture(hash->pk_GB_Previous_Color, gbuffers.previous.color);
        RHI::SetTexture(hash->pk_GB_Previous_Normals, gbuffers.previous.normals);
        RHI::SetTexture(hash->pk_GB_Previous_Depth, gbuffers.previous.depth);
        RHI::SetTexture(hash->pk_GB_Previous_DepthBiased, gbuffers.previous.depthBiased);
        RHI::SetBuffer(hash->pk_View, primaryView->constants);

        // Clear 'previous' targets if they've been reallocated 
        if (primaryView->IsResizeFrame())
        {
            graph->Clear(QueueType::Graphics, gbuffers.previous.color, {}, PK_FLOAT4_ZERO);
            graph->Clear(QueueType::Graphics, gbuffers.previous.normals, {}, float4(0.5f, 0.5f, 0.0f, 0.0f));
            graph->Clear(QueueType::Graphics, gbuffers.previous.depth, {}, TextureClearValue(PK_CLIPZ_FAR, 0u));
            graph->Clear(QueueType::Graphics, gbuffers.previous.depthBiased, {}, float4(PK_CLIPZ_FAR));
        }

        context->batcher->BeginCollectDrawCalls();
        {
            DispatchRenderPipelineEvent(graph, context, RenderPipelineEvent::CollectDraws);
            m_passLights.BuildLights(context);
        }
        context->batcher->EndCollectDrawCalls(graph);

        // Prune voxels & build AS.
        // These can happen before the end of last frame. 
        m_passSceneGI.PruneVoxels(graph, QueueType::Compute);
        context->cullingProxy->CullRayTracingGeometry(ScenePrimitiveFlags::DefaultMesh, AABB<float3>(), false, QueueType::Compute, m_sceneStructure.get());
        RHI::SetAccelerationStructure(hash->pk_SceneStructure, m_sceneStructure.get());

        // Async compute during last present.
        m_passFilmGrain.Compute(graph, QueueType::Compute);
        m_passLights.ComputeClusters(graph, QueueType::Compute, context);
        m_autoExposure.Render(graph, QueueType::Compute, context);
        m_depthOfField.ComputeAutoFocus(graph, QueueType::Compute, context);
        m_passSceneEnv.PreCompute(graph, QueueType::Compute, context);
        m_passSceneGI.VoxelMips(graph, QueueType::Compute);
        m_passSceneGI.ValidateReservoirs(graph, QueueType::Compute, context);

        // Depth pre pass. Meshlet cull based on prev frame hizb
        graph.SetRenderTarget({ gbuffers.current.depth, LoadOp::Clear, StoreOp::Store, { PK_CLIPZ_FAR, 0 } }, true);
        graph->SetStageExcludeMask(ShaderStageFlags::Fragment);
        DispatchRenderPipelineEvent(graph, context, RenderPipelineEvent::Depth);
        graph->SetStageExcludeMask(ShaderStageFlags::None);

        // Gbuffer pass & possible depth writes from invalid z culls.
        {
            auto targetDepth = RenderTargetBinding(gbuffers.current.depth, LoadOp::Load, StoreOp::None);
            auto targetNormals = RenderTargetBinding(gbuffers.current.normals, LoadOp::Clear, StoreOp::Store, float4(0.5f, 0.5f, 0.0f, 0.0f));
            auto targetDepthBiased = RenderTargetBinding(gbuffers.current.depthBiased, LoadOp::Clear, StoreOp::Store, PK_COLOR_CLEAR);
            graph.SetRenderTarget({ targetDepth, targetNormals, targetDepthBiased }, true);
            DispatchRenderPipelineEvent(graph, context, RenderPipelineEvent::GBuffer);
        }

        m_passHierarchicalDepth.Compute(graph, QueueType::Graphics, context);
        m_passVolumetricFog.ComputeDensity(graph, QueueType::Graphics, context);
        // Indirect GI ray tracing
        m_passSceneGI.DispatchRays(graph, QueueType::Compute, context);

        // Shadows, Voxelize scene & reproject gi
        m_passLights.RenderShadows(graph, context);
        m_passSceneGI.Voxelize(graph, context);
        m_passLights.RenderScreenSpaceShadows(graph, QueueType::Graphics, context);
        m_passSceneGI.ReprojectGI(graph, QueueType::Graphics, context);
        m_passVolumetricFog.Compute(graph, QueueType::Graphics, context);
        m_passSceneGI.RenderGI(graph, QueueType::Graphics, context);

        // Forward Opaque on graphics queue
        {
            auto targetDepth = RenderTargetBinding(gbuffers.current.depth, LoadOp::Load, StoreOp::None);
            auto targetColor = RenderTargetBinding(gbuffers.current.color, LoadOp::Clear, StoreOp::Store, PK_COLOR_CLEAR);
            graph.SetRenderTarget({ targetDepth, targetColor }, true);
            DispatchRenderPipelineEvent(graph, context, RenderPipelineEvent::ForwardOpaque);

            m_passSceneEnv.RenderBackground(graph, context);
            m_passVolumetricFog.Render(graph, QueueType::Graphics, gbuffers.current.color);

            DispatchRenderPipelineEvent(graph, context, RenderPipelineEvent::ForwardTransparent);
        }

        // Cache forward output of current frame
        graph->Blit(QueueType::Graphics, gbuffers.current.color, gbuffers.previous.color, {}, {}, FilterMode::Point);

        // Post Effects
        graph.BeginStatScope(hash->STAT_POSTFX, PK_COLOR_YELLOW);
        {
            // Previous color has been updated. leverage that and do taa without extra blit.
            m_temporalAntialiasing.Render(graph, QueueType::Graphics, context->views[0], gbuffers.previous.color, gbuffers.current.color);
            m_depthOfField.Render(graph, QueueType::Graphics, context, gbuffers.current.color);
            m_distort.Render(graph, QueueType::Graphics, context);
            m_bloom.Render(graph, QueueType::Graphics, context);
            m_passPostEffectsComposite.Render(graph, QueueType::Graphics, gbuffers.current.color);
        }
        graph.EndStatScope();

        DispatchRenderPipelineEvent(graph, context, RenderPipelineEvent::AfterPostEffects);

        primaryView->gbuffers.SwapBuffers();
    }
}
