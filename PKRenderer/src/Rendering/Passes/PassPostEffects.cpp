#include "PrecompiledHeader.h"
#include "PassPostEffects.h"
#include "Rendering/HashCache.h"
#include "Rendering/GraphicsAPI.h"
#include "Math/FunctionsColor.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace Utilities;
    using namespace Structs;
    using namespace Objects;

    PassPostEffects::PassPostEffects(AssetDatabase* assetDatabase, const ApplicationConfig* config) : 
        m_bloom(assetDatabase, config->InitialWidth, config->InitialHeight),
        m_histogram(assetDatabase),
        m_depthOfField(assetDatabase, config)
    {
        m_computeComposite = assetDatabase->Find<Shader>("CS_PostEffectsComposite");
        m_computeFilmGrain = assetDatabase->Find<Shader>("CS_FilmGrain");
        m_bloomLensDirtTexture = assetDatabase->Load<Texture>(config->FileBloomDirt.value.c_str());

        m_paramatersBuffer = CreateRef<ConstantBuffer>(BufferLayout(
        {
            {ElementType::Float, "pk_MinLogLuminance"},
            {ElementType::Float, "pk_InvLogLuminanceRange"},
            {ElementType::Float, "pk_LogLuminanceRange"},
            {ElementType::Float, "pk_TargetExposure"},
            {ElementType::Float, "pk_AutoExposureSpeed"},
            {ElementType::Float, "pk_BloomIntensity"},
            {ElementType::Float, "pk_BloomDirtIntensity"},
            {ElementType::Float, "pk_Vibrance"},
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
        }), "Color Grading Parameters");
        
        OnUpdateParameters(config);

        TextureDescriptor descriptor{};
        descriptor.format = TextureFormat::RGBA8;
        descriptor.resolution.x = 256;
        descriptor.resolution.y = 256;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;
        descriptor.sampler.wrap[0] = WrapMode::Repeat;
        descriptor.sampler.wrap[1] = WrapMode::Repeat;
        descriptor.sampler.wrap[2] = WrapMode::Repeat;
        descriptor.usage = TextureUsage::Default | TextureUsage::Storage;
        m_filmGrainTexture = Texture::Create(descriptor, "Film Grain Texture");
    }
    
    void PassPostEffects::Render(CommandBuffer* cmd, RenderTexture* destination, MemoryAccessFlags lastAccess)
    {
        cmd->BeginDebugScope("Post Effects", PK_COLOR_YELLOW);

        auto hash = HashCache::Get();
        auto color = destination->GetColor(0);
        auto grain = m_filmGrainTexture.get();
        
        cmd->SetBuffer(hash->pk_PostEffectsParams, m_paramatersBuffer->GetBuffer());

        cmd->SetImage(hash->_MainTex, grain, 0, 0);
        cmd->Dispatch(m_computeFilmGrain, { 16, 64, 1 });

        m_depthOfField.Execute(destination, lastAccess);
        m_bloom.Execute(destination, lastAccess);
        m_histogram.Execute(m_bloom.GetTexture(), MemoryAccessFlags::ComputeRead);

        cmd->SetImage(hash->_MainTex, color, 0, 0);
        cmd->SetTexture(hash->pk_BloomLensDirtTex, m_bloomLensDirtTexture);
        cmd->SetTexture(hash->pk_FilmGrainTex, grain);

        cmd->Barrier(grain, MemoryAccessFlags::ComputeWrite, MemoryAccessFlags::ComputeRead);

        auto res = destination->GetResolution();
        cmd->Dispatch(m_computeComposite, { (uint)glm::ceil(res.x / 16.0f), (uint)glm::ceil(res.y / 4.0f), 1u });

        cmd->EndDebugScope();
    }

    void PassPostEffects::OnUpdateParameters(const ApplicationConfig* config)
    {
        auto hash = HashCache::Get();

        m_paramatersBuffer->Set<float>(hash->pk_MinLogLuminance, config->AutoExposureLuminanceMin);
        m_paramatersBuffer->Set<float>(hash->pk_InvLogLuminanceRange, 1.0f / config->AutoExposureLuminanceRange);
        m_paramatersBuffer->Set<float>(hash->pk_LogLuminanceRange, config->AutoExposureLuminanceRange);
        m_paramatersBuffer->Set<float>(hash->pk_TargetExposure, config->TonemapExposure);
        m_paramatersBuffer->Set<float>(hash->pk_AutoExposureSpeed, config->AutoExposureSpeed);
        m_paramatersBuffer->Set<float>(hash->pk_BloomIntensity, glm::exp(config->BloomIntensity) - 1.0f);
        m_paramatersBuffer->Set<float>(hash->pk_BloomDirtIntensity, glm::exp(config->BloomLensDirtIntensity) - 1.0f);

        color lift, gamma, gain;
        Functions::GenerateLiftGammaGain(Functions::HexToRGB(config->CC_Shadows), Functions::HexToRGB(config->CC_Midtones), Functions::HexToRGB(config->CC_Highlights), &lift, &gamma, &gain);
        m_paramatersBuffer->Set<float>(hash->pk_Vibrance, config->CC_Vibrance);
        m_paramatersBuffer->Set<float4>(hash->pk_VignetteGrain, { config->VignetteIntensity, config->VignettePower, config->FilmGrainLuminance, config->FilmGrainIntensity });
        m_paramatersBuffer->Set<float4>(hash->pk_WhiteBalance, Functions::GetWhiteBalance(config->CC_TemperatureShift, config->CC_Tint));
        m_paramatersBuffer->Set<float4>(hash->pk_Lift, lift);
        m_paramatersBuffer->Set<float4>(hash->pk_Gamma, gamma);
        m_paramatersBuffer->Set<float4>(hash->pk_Gain, gain);
        m_paramatersBuffer->Set<float4>(hash->pk_ContrastGainGammaContribution, float4(config->CC_Contrast, config->CC_Gain, 1.0f / config->CC_Gamma, config->CC_Contribution));
        m_paramatersBuffer->Set<float4>(hash->pk_HSV, float4(config->CC_Hue, config->CC_Saturation, config->CC_Value, 1.0f));
        m_paramatersBuffer->Set<float4>(hash->pk_ChannelMixerRed, Functions::HexToRGB(config->CC_ChannelMixerRed));
        m_paramatersBuffer->Set<float4>(hash->pk_ChannelMixerGreen, Functions::HexToRGB(config->CC_ChannelMixerGreen));
        m_paramatersBuffer->Set<float4>(hash->pk_ChannelMixerBlue, Functions::HexToRGB(config->CC_ChannelMixerBlue));
        m_paramatersBuffer->FlushBuffer();
    }
}