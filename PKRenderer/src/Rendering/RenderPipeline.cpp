#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"
#include "ECS/Tokens/AccelerationStructureBuildToken.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsColor.h"

/*
TODO:
Fix roughness dominant factor usage
Test nvidias radius scaling
Investigate camera moving bias
Fix history fill fireflies & bad denoising quality (possibly 'dedirectionalize' diffuse?)
Fix ssrt miss hit dist
Write subject analysis
*/

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
        m_histogram(assetDatabase),
        m_batcher(),
        m_sequencer(sequencer),
        m_visibilityList(1024),
        m_resizeFrameIndex(0ull)
    {
        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.colorFormats[1] = TextureFormat::RGB10A2;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        m_renderTarget = CreateRef<RenderTexture>(descriptor, "Scene.RenderTarget");

        TextureDescriptor prevDesc{};
        prevDesc.format = TextureFormat::B10G11R11UF; // @TODO refactor to use RGB9E5 as this has very poor bit depth. needs a compute copy pass to work as RGB9E5 is not blittable.
        prevDesc.sampler.filterMin = FilterMode::Bilinear;
        prevDesc.sampler.filterMag = FilterMode::Bilinear;
        prevDesc.resolution = descriptor.resolution;
        m_previousColor = Texture::Create(prevDesc, "Scene.RenderTarget.Previous.Color0");

        prevDesc.format = TextureFormat::RGB10A2;
        m_previousNormals = Texture::Create(prevDesc, "Scene.RenderTarget.Previous.Color1");

        prevDesc.format = TextureFormat::Depth32F;
        m_previousDepth = Texture::Create(prevDesc, "Scene.RenderTarget.Previous.Depth");

        m_sceneStructure = AccelerationStructure::Create("Scene");

        auto hash = HashCache::Get();

        m_constantsPerFrame = CreateRef<ConstantBuffer>(BufferLayout(
            {
                { ElementType::Float4, hash->pk_Time },
                { ElementType::Float4, hash->pk_SinTime },
                { ElementType::Float4, hash->pk_CosTime },
                { ElementType::Float4, hash->pk_DeltaTime },
                { ElementType::Float4, hash->pk_CursorParams },
                { ElementType::Float4, hash->pk_WorldSpaceCameraPos },
                { ElementType::Float4, hash->pk_ViewSpaceCameraDelta },
                { ElementType::Float4, hash->pk_ProjectionParams },
                { ElementType::Float4, hash->pk_ExpProjectionParams },
                { ElementType::Float4, hash->pk_ScreenParams },
                { ElementType::Float4, hash->pk_ShadowCascadeZSplits },
                { ElementType::Float4, hash->pk_ProjectionJitter },
                { ElementType::Uint4, hash->pk_FrameRandom },
                { ElementType::Uint2, hash->pk_ScreenSize },
                { ElementType::Uint2, hash->pk_FrameIndex },
                { ElementType::Float4x4, hash->pk_MATRIX_V },
                { ElementType::Float4x4, hash->pk_MATRIX_I_V },
                { ElementType::Float4x4, hash->pk_MATRIX_P },
                { ElementType::Float4x4, hash->pk_MATRIX_I_P },
                { ElementType::Float4x4, hash->pk_MATRIX_VP },
                { ElementType::Float4x4, hash->pk_MATRIX_VP_N },
                { ElementType::Float4x4, hash->pk_MATRIX_I_VP },
                { ElementType::Float4x4, hash->pk_MATRIX_L_I_V },
                { ElementType::Float4x4, hash->pk_MATRIX_L_VP },
                { ElementType::Float4x4, hash->pk_MATRIX_L_VP_N },
                { ElementType::Float4x4, hash->pk_MATRIX_L_VP_D },
                { ElementType::Float, hash->pk_SceneEnv_Exposure }
            }), "Constants.Frame");

        m_constantsPostProcess = CreateRef<ConstantBuffer>(BufferLayout(
            {
                {ElementType::Float, "pk_MinLogLuminance"},
                {ElementType::Float, "pk_InvLogLuminanceRange"},
                {ElementType::Float, "pk_LogLuminanceRange"},
                {ElementType::Float, "pk_TargetExposure"},
                {ElementType::Float, "pk_AutoExposureSpeed"},
                {ElementType::Float, "pk_BloomIntensity"},
                {ElementType::Float, "pk_BloomDirtIntensity"},
                {ElementType::Float, "pk_Vibrance"},
                {ElementType::Float, "pk_TAA_Sharpness"},
                {ElementType::Float, "pk_TAA_BlendingStatic"},
                {ElementType::Float, "pk_TAA_BlendingMotion"},
                {ElementType::Float, "pk_TAA_MotionAmplification"},
                {ElementType::Float4, "pk_VignetteGrain"},
                {ElementType::Float4, "pk_WhiteBalance"},
                {ElementType::Float4, "pk_Lift"},
                {ElementType::Float4, "pk_Gamma"},
                {ElementType::Float4, "pk_Gain"},
                {ElementType::Float4, "pk_ContrastGainGammaContribution"},
                {ElementType::Float4, "pk_HSV"},
                {ElementType::Float4, "pk_ChannelMixerRed"},
                {ElementType::Float4, "pk_ChannelMixerGreen"},
                {ElementType::Float4, "pk_ChannelMixerBlue"},
            }), "Constants.PostProcess");

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
        sampler.mipMin = 0.0f;
        sampler.mipMax = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise256->SetSampler(sampler);

        sampler = bluenoise128x64->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.mipMin = 0.0f;
        sampler.mipMax = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise128x64->SetSampler(sampler);

        GraphicsAPI::SetTexture(hash->pk_Bluenoise256, bluenoise256);
        GraphicsAPI::SetTexture(hash->pk_Bluenoise128x64, bluenoise128x64);
        GraphicsAPI::SetTexture(hash->pk_LightCookies, lightCookies);
        GraphicsAPI::SetBuffer(hash->pk_PerFrameConstants, *m_constantsPerFrame.get());
        GraphicsAPI::SetBuffer(hash->pk_PostEffectsParams, *m_constantsPostProcess.get());
        PK_LOG_HEADER("----------RENDER PIPELINE INITIALIZED----------");
    }

    RenderPipeline::~RenderPipeline()
    {
        GraphicsAPI::GetActiveDriver()->WaitForIdle();
        m_constantsPerFrame = nullptr;
        m_renderTarget = nullptr;
    }

    void RenderPipeline::Step(PK::ECS::Tokens::ViewProjectionUpdateToken* token)
    {
        auto hash = HashCache::Get();

        // @TODO move to a sequencer step instead
        token->jitter = m_temporalAntialiasing.GetJitter();

        float2 jitter =
        {
            token->jitter.x / m_renderTarget->GetResolution().x,
            token->jitter.y / m_renderTarget->GetResolution().y
        };

        auto matrix_p_n = token->projection;
        auto matrix_p = Functions::GetPerspectiveJittered(matrix_p_n, jitter);
        auto matrix_v = token->view;
        auto matrix_i_v = glm::inverse(matrix_v);
        auto matrix_vp = matrix_p * matrix_v;
        auto matrix_vp_n = matrix_p_n * matrix_v;
        float n = m_znear = Functions::GetZNearFromProj(matrix_p);
        float f = m_zfar = Functions::GetZFarFromProj(matrix_p);

        auto matrix_l_i_v = matrix_i_v;
        auto matrix_l_vp = matrix_vp;
        auto matrix_l_vp_n = matrix_vp_n;

        m_constantsPerFrame->TryGet(hash->pk_MATRIX_I_V, matrix_l_i_v);

        if (m_constantsPerFrame->TryGet(hash->pk_MATRIX_VP, matrix_l_vp))
        {
            // We can assume that m_viewProjectionMatrix has been assigned if the cbuffer has values for this.
            matrix_l_vp_n = m_viewProjectionMatrix;
        }

        m_viewProjectionMatrix = matrix_vp_n;
        auto viewSpaceCameraDelta = matrix_v * float4(matrix_l_i_v[3].x, matrix_l_i_v[3].y, matrix_l_i_v[3].z, 1.0f);

        m_constantsPerFrame->Set<float4>(hash->pk_ProjectionParams, { n, f, f - n, 1.0f / f });
        m_constantsPerFrame->Set<float4>(hash->pk_ExpProjectionParams, { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n });
        m_constantsPerFrame->Set<float4>(hash->pk_WorldSpaceCameraPos, matrix_i_v[3]);
        m_constantsPerFrame->Set<float4>(hash->pk_ViewSpaceCameraDelta, viewSpaceCameraDelta);
        m_constantsPerFrame->Set<float4>(hash->pk_ProjectionJitter, token->jitter);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_V, matrix_v);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_V, matrix_i_v);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_P, matrix_p);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_P, glm::inverse(matrix_p));
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_VP, matrix_vp);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_VP_N, matrix_vp_n);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_VP, glm::inverse(matrix_vp));
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_L_I_V, matrix_l_i_v);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_L_VP, matrix_l_vp);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_L_VP_N, matrix_l_vp_n);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_L_VP_D, matrix_l_vp * matrix_i_v);
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

        if (m_renderTarget->Validate(resolution))
        {
            m_resizeFrameIndex = m_constantsPerFrame->Get<uint2>(hash->pk_FrameIndex)->x;
            m_constantsPerFrame->Set<uint2>(hash->pk_FrameIndex, { m_resizeFrameIndex, 0u });
        }

        m_previousColor->Validate(resolution);
        m_previousNormals->Validate(resolution);
        m_previousDepth->Validate(resolution);
        GraphicsAPI::SetTexture(hash->pk_ScreenDepthCurrent, m_renderTarget->GetDepth());
        GraphicsAPI::SetTexture(hash->pk_ScreenNormalsCurrent, m_renderTarget->GetColor(1));
        GraphicsAPI::SetTexture(hash->pk_ScreenDepthPrevious, m_previousDepth.get());
        GraphicsAPI::SetTexture(hash->pk_ScreenNormalsPrevious, m_previousNormals.get());
        GraphicsAPI::SetTexture(hash->pk_ScreenColorPrevious, m_previousColor.get());

        auto cascadeZSplits = m_passLights.GetCascadeZSplits(m_znear, m_zfar);
        m_constantsPerFrame->Set<float4>(hash->pk_ShadowCascadeZSplits, reinterpret_cast<float4*>(cascadeZSplits.planes));
        m_constantsPerFrame->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        m_constantsPerFrame->Set<uint2>(hash->pk_ScreenSize, { resolution.x, resolution.y });
        m_constantsPerFrame->FlushBuffer(QueueType::Transfer);

        auto cmdtransfer = queues->GetCommandBuffer(QueueType::Transfer);
        m_passSceneGI.PreRender(cmdtransfer, resolution);
        m_batcher.BeginCollectDrawCalls();
        {
            DispatchRenderEvent(cmdtransfer, Tokens::RenderEvent::CollectDraws, nullptr, &m_forwardPassGroup);
            m_passLights.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_znear, m_zfar);
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
        queues->Sync(QueueType::Transfer, QueueType::Graphics);
        queues->Sync(QueueType::Transfer, QueueType::Compute);

        // Only buffering needs to wait for previous results.
        // Eliminate redundant rendering waits by waiting for transfer instead.
        window->SetFrameFence(queues->GetFenceRef(QueueType::Transfer));

        // Concurrent Shadows & gbuffer
        cmdgraphics->SetRenderTarget(m_renderTarget.get(), { 1 }, true, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        cmdgraphics->ClearDepth(1.0f, 0u);

        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::GBuffer, "Forward.GBuffer", nullptr);
        m_passHierarchicalDepth.Compute(cmdgraphics, resolution);
        queues->Submit(QueueType::Graphics, &cmdgraphics);

        m_passFilmGrain.Compute(cmdcompute);
        m_passLights.ComputeClusters(cmdcompute);
        m_histogram.Render(cmdcompute, m_bloom.GetTexture());
        m_depthOfField.ComputeAutoFocus(cmdcompute, resolution.y);
        m_passVolumeFog.ComputeDensity(cmdcompute, resolution);
        m_passEnvBackground.ComputeSH(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        queues->Sync(QueueType::Graphics, QueueType::Compute);

        // Indirect GI ray tracing
        m_passSceneGI.DispatchRays(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        // Wait for misc async compute instead of ray dispatch
        queues->Sync(QueueType::Compute, QueueType::Graphics, -1);

        m_passLights.RenderShadows(cmdgraphics);

        // Voxelize scene & reproject gi
        m_passSceneGI.Preprocess(cmdgraphics, &m_batcher, m_forwardPassGroup);
        m_passVolumeFog.Compute(cmdgraphics, m_renderTarget->GetResolution());
        queues->Submit(QueueType::Graphics, &cmdgraphics);
        queues->Sync(QueueType::Graphics, QueueType::Compute);
        queues->Sync(QueueType::Compute, QueueType::Graphics);

        // Forward Opaque on graphics queue
        m_passSceneGI.RenderGI(cmdgraphics);
        cmdgraphics->SetRenderTarget(m_renderTarget.get(), { 0 }, true, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::ForwardOpaque, "Forward.Opaque", nullptr);
        
        m_passEnvBackground.RenderBackground(cmdgraphics);
        m_passVolumeFog.Render(cmdgraphics, m_renderTarget.get());

        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::ForwardTransparent, "Forward.Transparent", nullptr);

        // Cache forward output of current frame
        cmdgraphics->Blit(m_renderTarget->GetColor(0u), m_previousColor.get(), {}, {}, FilterMode::Point);

        // Post Effects
        cmdgraphics->BeginDebugScope("PostEffects", PK_COLOR_YELLOW);
        {
            m_temporalAntialiasing.Render(cmdgraphics, m_renderTarget.get());
            m_depthOfField.Render(cmdgraphics, m_renderTarget.get());
            m_bloom.Render(cmdgraphics, m_renderTarget.get());
            m_passPostEffectsComposite.Render(cmdgraphics, m_renderTarget.get());
        }
        cmdgraphics->EndDebugScope();

        DispatchRenderEvent(cmdgraphics, Tokens::RenderEvent::AfterPostEffects, "AfterPostEffects", nullptr);

        queues->Submit(QueueType::Graphics, &cmdgraphics);

        // Blit to window
        cmdgraphics->Blit(m_renderTarget->GetDepth(), m_previousDepth.get(), {}, {}, FilterMode::Point);
        cmdgraphics->Blit(m_renderTarget->GetColor(1), m_previousNormals.get(), {}, {}, FilterMode::Point);
        cmdgraphics->Blit(m_renderTarget->GetColor(0), window, FilterMode::Bilinear);
    }

    void RenderPipeline::Step(AssetImportToken<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto config = token->asset;

        m_constantsPerFrame->Set<float>(hash->pk_SceneEnv_Exposure, config->BackgroundExposure);

        m_constantsPostProcess->Set<float>(hash->pk_MinLogLuminance, config->AutoExposureLuminanceMin);
        m_constantsPostProcess->Set<float>(hash->pk_InvLogLuminanceRange, 1.0f / config->AutoExposureLuminanceRange);
        m_constantsPostProcess->Set<float>(hash->pk_LogLuminanceRange, config->AutoExposureLuminanceRange);
        m_constantsPostProcess->Set<float>(hash->pk_TargetExposure, config->TonemapExposure);
        m_constantsPostProcess->Set<float>(hash->pk_AutoExposureSpeed, config->AutoExposureSpeed);
        m_constantsPostProcess->Set<float>(hash->pk_BloomIntensity, glm::exp(config->BloomIntensity) - 1.0f);
        m_constantsPostProcess->Set<float>(hash->pk_BloomDirtIntensity, glm::exp(config->BloomLensDirtIntensity) - 1.0f);

        m_constantsPostProcess->Set<float>(hash->pk_TAA_Sharpness, config->TAASharpness);
        m_constantsPostProcess->Set<float>(hash->pk_TAA_BlendingStatic, config->TAABlendingStatic);
        m_constantsPostProcess->Set<float>(hash->pk_TAA_BlendingMotion, config->TAABlendingMotion);
        m_constantsPostProcess->Set<float>(hash->pk_TAA_MotionAmplification, config->TAAMotionAmplification);

        color lift, gamma, gain;
        Functions::GenerateLiftGammaGain(Functions::HexToRGB(config->CC_Shadows), Functions::HexToRGB(config->CC_Midtones), Functions::HexToRGB(config->CC_Highlights), &lift, &gamma, &gain);
        m_constantsPostProcess->Set<float>(hash->pk_Vibrance, config->CC_Vibrance);
        m_constantsPostProcess->Set<float4>(hash->pk_VignetteGrain, { config->VignetteIntensity, config->VignettePower, config->FilmGrainLuminance, config->FilmGrainIntensity });
        m_constantsPostProcess->Set<float4>(hash->pk_WhiteBalance, Functions::GetWhiteBalance(config->CC_TemperatureShift, config->CC_Tint));
        m_constantsPostProcess->Set<float4>(hash->pk_Lift, lift);
        m_constantsPostProcess->Set<float4>(hash->pk_Gamma, gamma);
        m_constantsPostProcess->Set<float4>(hash->pk_Gain, gain);
        m_constantsPostProcess->Set<float4>(hash->pk_ContrastGainGammaContribution, float4(config->CC_Contrast, config->CC_Gain, 1.0f / config->CC_Gamma, config->CC_Contribution));
        m_constantsPostProcess->Set<float4>(hash->pk_HSV, float4(config->CC_Hue, config->CC_Saturation, config->CC_Value, 1.0f));
        m_constantsPostProcess->Set<float4>(hash->pk_ChannelMixerRed, Functions::HexToRGB(config->CC_ChannelMixerRed));
        m_constantsPostProcess->Set<float4>(hash->pk_ChannelMixerGreen, Functions::HexToRGB(config->CC_ChannelMixerGreen));
        m_constantsPostProcess->Set<float4>(hash->pk_ChannelMixerBlue, Functions::HexToRGB(config->CC_ChannelMixerBlue));
        m_constantsPostProcess->FlushBuffer(QueueType::Transfer);

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

        auto token = Tokens::TokenRenderEvent(cmd, m_renderTarget.get(), &m_visibilityList, &m_batcher, m_viewProjectionMatrix, m_znear, m_zfar);

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