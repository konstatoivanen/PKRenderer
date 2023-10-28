#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"
#include "ECS/Tokens/AccelerationStructureBuildToken.h"
#include "Math/FunctionsMisc.h"

namespace PK::Rendering
{
    using namespace Utilities;
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace ECS;
    using namespace Objects;
    using namespace Structs;

    RenderPipeline::RenderPipeline(AssetDatabase* assetDatabase, EntityDatabase* entityDb, Sequencer* sequencer, ApplicationConfig* config) :
        m_passHierarchicalDepth(assetDatabase, config),
        m_passEnvBackground(assetDatabase),
        m_passPostEffectsComposite(assetDatabase, config),
        m_passLights(assetDatabase, entityDb, sequencer, &m_batcher, config),
        m_passSceneGI(assetDatabase, config),
        m_passVolumeFog(assetDatabase, config),
        m_passFilmGrain(assetDatabase),
        m_depthOfField(assetDatabase, config),
        m_temporalAntialiasing(assetDatabase, config->InitialWidth, config->InitialHeight),
        m_bloom(assetDatabase, config->InitialWidth, config->InitialHeight),
        m_autoExposure(assetDatabase),
        m_batcher(),
        m_sequencer(sequencer),
        m_visibilityList(1024),
        m_resizeFrameIndex(0ull)
    {
        TextureDescriptor curDesc{};
        curDesc.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        curDesc.sampler.filterMin = FilterMode::Bilinear;
        curDesc.sampler.filterMag = FilterMode::Bilinear;
        
        curDesc.format = TextureFormat::RGBA16F;
        curDesc.usage = TextureUsage::RTColorSample | TextureUsage::Storage;
        m_gbuffers.current.color = Texture::Create(curDesc, "Scene.RenderTarget.Color");

        curDesc.format = TextureFormat::RGB10A2;
        curDesc.usage = TextureUsage::RTColorSample;
        m_gbuffers.current.normals = Texture::Create(curDesc, "Scene.RenderTarget.Normals");

        curDesc.format = TextureFormat::Depth32F;
        curDesc.usage = TextureUsage::RTDepthSample;
        m_gbuffers.current.depth = Texture::Create(curDesc, "Scene.RenderTarget.Depth");

        // @TODO refactor to use RGB9E5 as this has very poor bit depth. needs a compute copy pass to work as RGB9E5 is not blittable.
        TextureDescriptor prevDesc{};
        prevDesc.format = TextureFormat::B10G11R11UF; 
        prevDesc.sampler.filterMin = FilterMode::Bilinear;
        prevDesc.sampler.filterMag = FilterMode::Bilinear;
        prevDesc.resolution = curDesc.resolution;
        m_gbuffers.previous.color = Texture::Create(prevDesc, "Scene.RenderTarget.Previous.Color");

        prevDesc.format = TextureFormat::RGB10A2;
        m_gbuffers.previous.normals = Texture::Create(prevDesc, "Scene.RenderTarget.Previous.Normals");

        prevDesc.format = TextureFormat::Depth32F;
        m_gbuffers.previous.depth = Texture::Create(prevDesc, "Scene.RenderTarget.Previous.Depth");

        m_sceneStructure = AccelerationStructure::Create("Scene");

        auto hash = HashCache::Get();

        m_constantsPerFrame = CreateRef<ConstantBuffer>(BufferLayout(
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
                { ElementType::Float4, hash->pk_WorldSpaceCameraPos },
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
            }), 
            "Constants.Frame");

        AssetImportToken<ApplicationConfig> token{ assetDatabase, config };
        Step(&token);

        auto bluenoise256 = assetDatabase->Load<Texture>("res/textures/default/T_Bluenoise256.ktx2");
        auto bluenoise128x64 = assetDatabase->Load<Texture>("res/textures/default/T_Bluenoise128x64.ktx2");
        auto lightCookies = assetDatabase->Load<Texture>("res/textures/default/T_LightCookies.ktx2");

        auto sampler = lightCookies->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Clamp;
        sampler.wrap[1] = WrapMode::Clamp;
        sampler.wrap[2] = WrapMode::Clamp;
        lightCookies->SetSampler(sampler);

        sampler = bluenoise256->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise256->SetSampler(sampler);

        sampler = bluenoise128x64->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise128x64->SetSampler(sampler);

        GraphicsAPI::SetTexture(hash->pk_Bluenoise256, bluenoise256);
        GraphicsAPI::SetTexture(hash->pk_Bluenoise128x64, bluenoise128x64);
        GraphicsAPI::SetTexture(hash->pk_LightCookies, lightCookies);
        GraphicsAPI::SetBuffer(hash->pk_PerFrameConstants, *m_constantsPerFrame.get());

        Structs::SamplerDescriptor samplerDesc{};
        samplerDesc.anisotropy = 16.0f;
        samplerDesc.filterMin = FilterMode::Trilinear;
        samplerDesc.filterMag = FilterMode::Trilinear;
        samplerDesc.wrap[0] = WrapMode::Repeat;
        samplerDesc.wrap[1] = WrapMode::Repeat;
        samplerDesc.wrap[2] = WrapMode::Repeat;
        GraphicsAPI::SetSampler(hash->pk_Sampler_SurfDefault, samplerDesc);

        samplerDesc.anisotropy = 0.0f;
        samplerDesc.filterMin = FilterMode::Bilinear;
        samplerDesc.filterMag = FilterMode::Bilinear;
        samplerDesc.wrap[0] = WrapMode::Clamp;
        samplerDesc.wrap[1] = WrapMode::Clamp;
        samplerDesc.wrap[2] = WrapMode::Clamp;
        GraphicsAPI::SetSampler(hash->pk_Sampler_GBuffer, samplerDesc);

        PK_LOG_HEADER("----------RENDER PIPELINE INITIALIZED----------");
    }

    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_constantsPerFrame = nullptr;
        m_sceneStructure = nullptr;
        m_gbuffers = {};
    }

    void RenderPipeline::Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token)
    {
        auto hash = HashCache::Get();

        // @TODO move to a sequencer step instead
        token->jitter = m_temporalAntialiasing.GetJitter();

        float2 jitter =
        {
            token->jitter.x / m_gbuffers.current.color->GetResolution().x,
            token->jitter.y / m_gbuffers.current.color->GetResolution().y
        };

        auto viewToClipNoJitter = token->viewToClip;
        auto viewToClip = Functions::GetPerspectiveJittered(viewToClipNoJitter, jitter);
        auto clipToView = glm::inverse(viewToClip);
        auto worldToView = token->worldToView;
        auto viewToWorld = glm::inverse(worldToView);
        auto worldToClip = viewToClip * worldToView;
        auto worldToClipNoJitter = viewToClipNoJitter * worldToView;
        float n = m_znear = Functions::GetZNearFromClip(viewToClip);
        float f = m_zfar = Functions::GetZFarFromClip(viewToClip);

        auto viewToWorldPrev = Functions::TransposeTo3x4(viewToWorld);
        auto worldToClipPrev = worldToClip;
        auto worldToClipPrevNoJitter = worldToClipNoJitter;

        m_constantsPerFrame->TryGet(hash->pk_ViewToWorld, viewToWorldPrev);

        if (m_constantsPerFrame->TryGet(hash->pk_WorldToClip, worldToClipPrev))
        {
            // We can assume that m_worldToClip has been assigned if the cbuffer has values for this.
            worldToClipPrevNoJitter = m_worldToClip;
        }

        m_worldToClip = worldToClipNoJitter;
        auto viewSpaceCameraDelta = worldToView * float4(viewToWorldPrev[0].w, viewToWorldPrev[1].w, viewToWorldPrev[2].w, 1.0f);

        m_constantsPerFrame->Set<float4>(hash->pk_ClipParams, { n, f, viewToClip[2][2], viewToClip[3][2] });
        m_constantsPerFrame->Set<float4>(hash->pk_ClipParamsInv, { clipToView[0][0], clipToView[1][1], clipToView[2][3], clipToView[3][3] });
        m_constantsPerFrame->Set<float4>(hash->pk_ClipParamsExp, { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n });
        m_constantsPerFrame->Set<float4>(hash->pk_WorldSpaceCameraPos, viewToWorld[3]);
        m_constantsPerFrame->Set<float4>(hash->pk_ViewSpaceCameraDelta, viewSpaceCameraDelta);
        m_constantsPerFrame->Set<float4>(hash->pk_ProjectionJitter, token->jitter);
        m_constantsPerFrame->Set<float3x4>(hash->pk_WorldToView, Functions::TransposeTo3x4(worldToView));
        m_constantsPerFrame->Set<float3x4>(hash->pk_ViewToWorld, Functions::TransposeTo3x4(viewToWorld));
        m_constantsPerFrame->Set<float3x4>(hash->pk_ViewToWorldPrev, viewToWorldPrev);
        m_constantsPerFrame->Set<float4x4>(hash->pk_ViewToClip, viewToClip);
        m_constantsPerFrame->Set<float4x4>(hash->pk_WorldToClip, worldToClip);
        m_constantsPerFrame->Set<float4x4>(hash->pk_WorldToClip_NoJitter, worldToClipNoJitter);
        m_constantsPerFrame->Set<float4x4>(hash->pk_WorldToClipPrev, worldToClipPrev);
        m_constantsPerFrame->Set<float4x4>(hash->pk_WorldToClipPrev_NoJitter, worldToClipPrevNoJitter);
        m_constantsPerFrame->Set<float4x4>(hash->pk_ViewToClipDelta, worldToClipPrev * viewToWorld);
    }

    void RenderPipeline::Step(PK::ECS::Tokens::TimeToken* token)
    {
        auto* hash = HashCache::Get();
        m_constantsPerFrame->Set<float4>(hash->pk_Time, { (float)token->time / 20.0f, (float)token->time, (float)token->time * 2.0f, (float)token->time * 3.0f });
        m_constantsPerFrame->Set<float4>(hash->pk_SinTime, { (float)sin(token->time / 8.0f), (float)sin(token->time / 4.0f), (float)sin(token->time / 2.0f), (float)sin(token->time) });
        m_constantsPerFrame->Set<float4>(hash->pk_CosTime, { (float)cos(token->time / 8.0f), (float)cos(token->time / 4.0f), (float)cos(token->time / 2.0f), (float)cos(token->time) });
        m_constantsPerFrame->Set<float4>(hash->pk_DeltaTime, { (float)token->deltaTime, 1.0f / (float)token->deltaTime, (float)token->smoothDeltaTime, 1.0f / (float)token->smoothDeltaTime });
        m_constantsPerFrame->Set<uint2>(hash->pk_FrameIndex, { token->frameIndex % 0xFFFFFFFFu, (token->frameIndex - m_resizeFrameIndex) % 0xFFFFFFFFu });
        m_constantsPerFrame->Set<uint4>(hash->pk_FrameRandom, Functions::MurmurHash41((uint32_t)(token->frameIndex % ~0u)));
        token->logFrameRate = true;
    }

    void RenderPipeline::Step(Window* window, int condition)
    {
        auto hash = HashCache::Get();
        auto queues = GraphicsAPI::GetQueues();
        auto resolution = window->GetResolutionAligned();
        auto gbuffers = m_gbuffers.GetView();

        if (m_gbuffers.Validate(resolution))
        {
            m_resizeFrameIndex = m_constantsPerFrame->Get<uint2>(hash->pk_FrameIndex)->x;
            m_constantsPerFrame->Set<uint2>(hash->pk_FrameIndex, { m_resizeFrameIndex, 0u });
        }

        GraphicsAPI::SetTexture(hash->pk_GB_Current_Normals, gbuffers.current.normals);
        GraphicsAPI::SetTexture(hash->pk_GB_Current_Depth, gbuffers.current.depth);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_Color, gbuffers.previous.color);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_Normals, gbuffers.previous.normals);
        GraphicsAPI::SetTexture(hash->pk_GB_Previous_Depth, gbuffers.previous.depth);

        auto cascadeZSplits = m_passLights.GetCascadeZSplits(m_znear, m_zfar);
        m_constantsPerFrame->Set<float4>(hash->pk_ShadowCascadeZSplits, reinterpret_cast<float4*>(cascadeZSplits.data()));
        m_constantsPerFrame->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        m_constantsPerFrame->Set<uint2>(hash->pk_ScreenSize, { resolution.x, resolution.y });
        m_constantsPerFrame->FlushBuffer(QueueType::Transfer);

        auto cmdtransfer = queues->GetCommandBuffer(QueueType::Transfer);
        m_passSceneGI.PreRender(cmdtransfer, resolution);
        m_batcher.BeginCollectDrawCalls();
        {
            DispatchRenderEvent(cmdtransfer, Tokens::RenderEvent::CollectDraws, nullptr, &m_forwardPassGroup);
            m_passLights.Cull(this, &m_visibilityList, m_worldToClip, m_znear, m_zfar);
        }
        m_batcher.EndCollectDrawCalls(cmdtransfer);

        auto* cmdgraphics = queues->GetCommandBuffer(QueueType::Graphics);
        auto* cmdcompute = queues->GetCommandBuffer(QueueType::Compute);

        // Prune voxels & build AS.
        // These can happen before the end of last frame. 
        m_passSceneGI.PruneVoxels(cmdcompute);
        m_sequencer->NextEmplace<Tokens::TokenAccelerationStructureBuild>
        (
            this, 0, QueueType::Compute, m_sceneStructure.get(), RenderableFlags::DefaultMesh, BoundingBox(), false
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
        window->SetFrameFence(queues->GetFenceRef(QueueType::Transfer));

        // Concurrent Shadows & gbuffer
        cmdgraphics->SetRenderTarget({ gbuffers.current.depth, gbuffers.current.normals }, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        cmdgraphics->ClearDepth(PK_CLIPZ_FAR, 0u);

        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::GBuffer, "Forward.GBuffer", nullptr);
        m_passHierarchicalDepth.Compute(cmdgraphics, resolution);
        queues->Submit(QueueType::Graphics, &cmdgraphics);

        // Async compute during last present.
        m_passFilmGrain.Compute(cmdcompute);
        m_passLights.ComputeClusters(cmdcompute, resolution);
        m_autoExposure.Render(cmdcompute, m_bloom.GetTexture());
        m_depthOfField.ComputeAutoFocus(cmdcompute, resolution.y);
        m_passVolumeFog.ComputeDensity(cmdcompute, resolution);
        m_passEnvBackground.ComputeSH(cmdcompute);
        m_passSceneGI.ValidateReservoirs(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        queues->Sync(QueueType::Graphics, QueueType::Compute);

        // Indirect GI ray tracing
        m_passSceneGI.DispatchRays(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        // Wait for misc async compute instead of ray dispatch
        queues->Sync(QueueType::Compute, QueueType::Graphics, -1);

        // Shadows, Voxelize scene & reproject gi
        m_passLights.RenderShadows(cmdgraphics, resolution);
        m_passSceneGI.Preprocess(cmdgraphics, &m_batcher, m_forwardPassGroup);
        m_passVolumeFog.Compute(cmdgraphics, gbuffers.current.color->GetResolution());
        queues->Submit(QueueType::Graphics, &cmdgraphics);
        queues->Transfer(QueueType::Graphics, QueueType::Compute);
        queues->Wait(QueueType::Compute, QueueType::Graphics);

        // Forward Opaque on graphics queue
        m_passSceneGI.RenderGI(cmdgraphics);
        cmdgraphics->SetRenderTarget({ gbuffers.current.depth, gbuffers.current.color }, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::ForwardOpaque, "Forward.Opaque", nullptr);
        
        m_passEnvBackground.RenderBackground(cmdgraphics);
        m_passVolumeFog.Render(cmdgraphics, gbuffers.current.color);

        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::ForwardTransparent, "Forward.Transparent", nullptr);

        // Cache forward output of current frame
        cmdgraphics->Blit(gbuffers.current.color, gbuffers.previous.color, {}, {}, FilterMode::Point);

        // Post Effects
        cmdgraphics->BeginDebugScope("PostEffects", PK_COLOR_YELLOW);
        {
            m_temporalAntialiasing.Render(cmdgraphics, gbuffers.current.color);
            m_depthOfField.Render(cmdgraphics, gbuffers.current.color);
            m_bloom.Render(cmdgraphics, gbuffers.current.color);
            m_passPostEffectsComposite.Render(cmdgraphics, gbuffers.current.color);
        }
        cmdgraphics->EndDebugScope();

        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::AfterPostEffects, "AfterPostEffects", nullptr);

        queues->Submit(QueueType::Graphics, &cmdgraphics);

        // Blit to window
        cmdgraphics->Blit(gbuffers.current.depth, gbuffers.previous.depth, {}, {}, FilterMode::Point);
        cmdgraphics->Blit(gbuffers.current.normals, gbuffers.previous.normals, {}, {}, FilterMode::Point);
        cmdgraphics->Blit(gbuffers.current.color, window, FilterMode::Bilinear);
    }

    void RenderPipeline::Step(AssetImportToken<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto config = token->asset;
        m_constantsPerFrame->Set<float>(hash->pk_SceneEnv_Exposure, config->BackgroundExposure);
        m_passPostEffectsComposite.OnUpdateParameters(token);
        m_passEnvBackground.OnUpdateParameters(token);
        m_passSceneGI.OnUpdateParameters(config);
        m_depthOfField.OnUpdateParameters(config);
        m_passVolumeFog.OnUpdateParameters(config);
    }

    void RenderPipeline::DispatchRenderEvent(Objects::CommandBuffer* cmd, ECS::Tokens::RenderEvent renderEvent, const char* name, uint32_t* outPassGroup)
    {
        if (name != nullptr)
        {
            cmd->BeginDebugScope(name, PK_COLOR_GREEN);
        }

        auto token = Tokens::TokenRenderEvent(cmd, m_gbuffers.GetView(), &m_visibilityList, &m_batcher, m_worldToClip, m_znear, m_zfar);

        m_sequencer->Next<Tokens::TokenRenderEvent>(this, &token, (int)renderEvent);
        
        if (outPassGroup != nullptr)
        {
            *outPassGroup = token.outPassGroup;
        }

        if (name != nullptr)
        {
            cmd->EndDebugScope();
        }
    }
}