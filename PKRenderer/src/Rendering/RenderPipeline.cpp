#include "PrecompiledHeader.h"
#include "Core/Services/Log.h"
#include "RenderPipeline.h"
#include "Rendering/MeshUtility.h"
#include "Rendering/HashCache.h"
#include "ECS/Contextual/Tokens/AccelerationStructureBuildToken.h"
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsColor.h"

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
        m_passPostEffectsComposite(assetDatabase, config),
        m_passGeometry(entityDb, sequencer, &m_batcher),
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
        m_visibilityList(1024)
    {
        m_OEMBackgroundShader = assetDatabase->Find<Shader>("SH_VS_IBLBackground");

        RenderTextureDescriptor descriptor{};
        descriptor.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        descriptor.colorFormats[0] = TextureFormat::RGBA16F;
        descriptor.colorFormats[1] = TextureFormat::RGBA16F;
        descriptor.depthFormat = TextureFormat::Depth32F;
        descriptor.usage = TextureUsage::Sample | TextureUsage::Storage;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        m_renderTarget = CreateRef<RenderTexture>(descriptor, "Scene.RenderTarget");

        TextureDescriptor depthDescriptor{};
        depthDescriptor.resolution = { config->InitialWidth, config->InitialHeight, 1 };
        depthDescriptor.format = TextureFormat::Depth32F;
        depthDescriptor.usage = TextureUsage::RTDepthSample;
        depthDescriptor.sampler.filterMin = FilterMode::Bilinear;
        depthDescriptor.sampler.filterMag = FilterMode::Bilinear;
        m_depthPrevious = Texture::Create(depthDescriptor, "Scene.DepthTexture.Previous");

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
            { ElementType::Float4, hash->pk_ProjectionParams },
            { ElementType::Float4, hash->pk_ExpProjectionParams },
            { ElementType::Float4, hash->pk_ScreenParams },
            { ElementType::Float4, hash->pk_ShadowCascadeZSplits },
            { ElementType::Float4, hash->pk_ProjectionJitter },
            { ElementType::Float4x4, hash->pk_MATRIX_V },
            { ElementType::Float4x4, hash->pk_MATRIX_I_V },
            { ElementType::Float4x4, hash->pk_MATRIX_P },
            { ElementType::Float4x4, hash->pk_MATRIX_I_P },
            { ElementType::Float4x4, hash->pk_MATRIX_VP },
            { ElementType::Float4x4, hash->pk_MATRIX_I_VP },
            { ElementType::Float4x4, hash->pk_MATRIX_L_VP },
            { ElementType::Float4x4, hash->pk_MATRIX_LD_P },
            { ElementType::Float, hash->pk_SceneOEM_Exposure },
            { ElementType::Uint, hash->pk_FrameIndex }
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

        AssetImportToken<ApplicationConfig> token { assetDatabase, config };
        Step(&token);

        auto bluenoise = assetDatabase->Load<Texture>("res/textures/default/T_Bluenoise256.ktx2");
        auto lightCookies = assetDatabase->Load<Texture>("res/textures/default/T_LightCookies.ktx2");
        
        auto sampler = lightCookies->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Clamp;
        sampler.wrap[1] = WrapMode::Clamp;
        sampler.wrap[2] = WrapMode::Clamp;
        lightCookies->SetSampler(sampler);

        sampler = bluenoise->GetSamplerDescriptor();
        sampler.anisotropy = 0.0f;
        sampler.mipMin = 0.0f;
        sampler.mipMax = 0.0f;
        sampler.filterMin = FilterMode::Point;
        sampler.filterMag = FilterMode::Bilinear;
        bluenoise->SetSampler(sampler);

        GraphicsAPI::SetTexture(hash->pk_Bluenoise256, bluenoise);
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

        token->projection = Functions::GetPerspectiveJittered(token->projection, jitter);

        auto cameraMatrix = glm::inverse(token->view);

        auto n = Functions::GetZNearFromProj(token->projection);
        auto f = Functions::GetZFarFromProj(token->projection);
        auto vp = token->projection * token->view;
        auto pvp = vp;

        m_viewProjectionMatrix = vp;
        m_znear = n;
        m_zfar = f;

        m_constantsPerFrame->TryGet(hash->pk_MATRIX_VP, pvp);
        m_constantsPerFrame->Set<float4>(hash->pk_ProjectionParams, { n, f, f - n, 1.0f / f });
        m_constantsPerFrame->Set<float4>(hash->pk_ExpProjectionParams, { 1.0f / glm::log2(f / n), -log2(n) / log2(f / n), f / n, 1.0f / n });
        m_constantsPerFrame->Set<float4>(hash->pk_WorldSpaceCameraPos, cameraMatrix[3]);
        m_constantsPerFrame->Set<float4>(hash->pk_ProjectionJitter, token->jitter);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_V, token->view);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_V, cameraMatrix);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_P, token->projection);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_P, glm::inverse(token->projection));
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_VP, vp);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_I_VP, glm::inverse(vp));
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_L_VP, pvp);
        m_constantsPerFrame->Set<float4x4>(hash->pk_MATRIX_LD_P, pvp * cameraMatrix);
    }

    void RenderPipeline::Step(PK::ECS::Tokens::TimeToken* token)
    {
        auto* hash = HashCache::Get();
        m_constantsPerFrame->Set<float4>(hash->pk_Time, { (float)token->time / 20.0f, (float)token->time, (float)token->time * 2.0f, (float)token->time * 3.0f });
        m_constantsPerFrame->Set<float4>(hash->pk_SinTime, { (float)sin(token->time / 8.0f), (float)sin(token->time / 4.0f), (float)sin(token->time / 2.0f), (float)sin(token->time) });
        m_constantsPerFrame->Set<float4>(hash->pk_CosTime, { (float)cos(token->time / 8.0f), (float)cos(token->time / 4.0f), (float)cos(token->time / 2.0f), (float)cos(token->time) });
        m_constantsPerFrame->Set<float4>(hash->pk_DeltaTime, { (float)token->deltaTime, 1.0f / (float)token->deltaTime, (float)token->smoothDeltaTime, 1.0f / (float)token->smoothDeltaTime });
        m_constantsPerFrame->Set<uint>(hash->pk_FrameIndex, token->frameIndex % 0xFFFFFFFFu);
        token->logFrameRate = true;
    }
    
    void RenderPipeline::Step(Window* window, int condition)
    {
        auto hash = HashCache::Get();
        auto queues = GraphicsAPI::GetQueues();
        auto resolution = window->GetResolution();

        m_renderTarget->Validate(resolution);
        m_depthPrevious->Validate(resolution);

        GraphicsAPI::SetTexture(hash->pk_ScreenDepthCurrent, m_renderTarget->GetDepth());
        GraphicsAPI::SetTexture(hash->pk_ScreenDepthPrevious, m_depthPrevious.get());
        GraphicsAPI::SetTexture(hash->pk_ScreenNormals, m_renderTarget->GetColor(1));

        auto cascadeZSplits = m_passLights.GetCascadeZSplits(m_znear, m_zfar);
        m_constantsPerFrame->Set<float4>(hash->pk_ShadowCascadeZSplits, reinterpret_cast<float4*>(cascadeZSplits.planes));
        m_constantsPerFrame->Set<float4>(hash->pk_ScreenParams, { (float)resolution.x, (float)resolution.y, 1.0f / (float)resolution.x, 1.0f / (float)resolution.y });
        m_constantsPerFrame->FlushBuffer(QueueType::Transfer);

        auto cmdtransfer = queues->GetCommandBuffer(QueueType::Transfer);
        m_passSceneGI.PreRender(cmdtransfer, resolution);

        m_batcher.BeginCollectDrawCalls();
        m_passGeometry.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_zfar - m_znear);
        m_passLights.Cull(this, &m_visibilityList, m_viewProjectionMatrix, m_znear, m_zfar);
        m_batcher.EndCollectDrawCalls(cmdtransfer);
        
        // End transfer operations
        queues->Sync(QueueType::Graphics, QueueType::Transfer);
        queues->Submit(QueueType::Transfer);
        queues->Sync(QueueType::Transfer, QueueType::Graphics);
        queues->Sync(QueueType::Transfer, QueueType::Compute);

        // Only buffering needs to wait for previous results.
        // Eliminate redundant rendering waits by waiting for transfer instead.
        window->SetFrameFence(queues->GetFenceRef(QueueType::Transfer));

        auto* cmdgraphics = queues->GetCommandBuffer(QueueType::Graphics);
        auto* cmdcompute = queues->GetCommandBuffer(QueueType::Compute);

        // Concurrent Shadows & gbuffer
        m_passLights.RenderShadows(cmdgraphics);
        cmdgraphics->SetRenderTarget(m_renderTarget.get(), { 1 }, true, true);
        cmdgraphics->ClearColor({ 0, 0, -1.0f, 1.0f }, 0);
        cmdgraphics->ClearDepth(1.0f, 0u);
        m_passGeometry.RenderGBuffer(cmdgraphics);
        queues->Submit(QueueType::Graphics, &cmdgraphics);

        // Async Work Noise & Light Assignment
        m_passFilmGrain.Compute(cmdcompute);
        m_passLights.ComputeClusters(cmdcompute);
        m_passSceneGI.PruneVoxels(cmdcompute);

        // Build acceleration structures on the async queue
        Tokens::AccelerationStructureBuildToken token{ QueueType::Compute, m_sceneStructure.get(), RenderableFlags::DefaultMesh, {}, false };
        m_sequencer->Next<Tokens::AccelerationStructureBuildToken>(this, &token);
        GraphicsAPI::SetAccelerationStructure(hash->pk_SceneStructure, m_sceneStructure.get());
        queues->Submit(QueueType::Compute, &cmdcompute);
        queues->Sync(QueueType::Compute, QueueType::Graphics);
        queues->Sync(QueueType::Graphics, QueueType::Compute);

        m_passVolumeFog.ComputeDepthTiles(cmdcompute, m_renderTarget->GetResolution());
        queues->Submit(QueueType::Compute, &cmdcompute);

        // Voxelize, subsequent passes depend on this
        m_passSceneGI.RenderVoxels(cmdgraphics, &m_batcher, m_passGeometry.GetPassGroup());
        queues->Submit(QueueType::Graphics, &cmdgraphics);
        queues->Sync(QueueType::Graphics, QueueType::Compute);

        // Forward Opaque on graphics queue
        m_passSceneGI.RenderGI(cmdgraphics);
        cmdgraphics->SetRenderTarget(m_renderTarget.get(), { 0 }, true, true);
        cmdgraphics->ClearColor(PK_COLOR_CLEAR, 0);
        m_passGeometry.RenderForward(cmdgraphics);
        cmdgraphics->Blit(m_OEMBackgroundShader);
        queues->Submit(QueueType::Graphics, &cmdgraphics);

        // Compute voxel volumes on async queue
        m_passVolumeFog.Compute(cmdcompute);
        queues->Submit(QueueType::Compute, &cmdcompute);
        queues->Sync(QueueType::Compute, QueueType::Graphics);

        // Forward Transparent on graphics queue
        m_passVolumeFog.Render(cmdgraphics, m_renderTarget.get());

        // Post Effects
        cmdgraphics->BeginDebugScope("PostEffects", PK_COLOR_YELLOW);
        m_temporalAntialiasing.Render(cmdgraphics, m_renderTarget.get());
        m_depthOfField.Render(cmdgraphics, m_renderTarget.get());
        m_bloom.Render(cmdgraphics, m_renderTarget.get());
        m_histogram.Render(cmdgraphics, m_bloom.GetTexture());
        m_passPostEffectsComposite.Render(cmdgraphics, m_renderTarget.get());
        cmdgraphics->EndDebugScope();

        // Blit to window
        cmdgraphics->Blit(m_renderTarget->GetDepth(), m_depthPrevious.get(), {}, {}, FilterMode::Point);
        cmdgraphics->Blit(m_renderTarget->GetColor(0), window, FilterMode::Bilinear);
    }

    void RenderPipeline::Step(AssetImportToken<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto config = token->asset;

        auto tex = token->assetDatabase->Load<Texture>(token->asset->FileBackgroundTexture.value.c_str());
        auto sampler = tex->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Mirror;
        sampler.wrap[1] = WrapMode::Mirror;
        sampler.wrap[2] = WrapMode::Mirror;
        tex->SetSampler(sampler);
        GraphicsAPI::SetTexture(hash->pk_SceneOEM_HDR, tex);

        m_constantsPerFrame->Set<float>(hash->pk_SceneOEM_Exposure, config->BackgroundExposure);

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

        m_depthOfField.OnUpdateParameters(config);
        m_passVolumeFog.OnUpdateParameters(config);
    }
}