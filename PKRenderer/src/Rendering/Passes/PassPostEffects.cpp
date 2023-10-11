#include "PrecompiledHeader.h"
#include "PassPostEffects.h"
#include "Rendering/HashCache.h"
#include "Math/FunctionsColor.h"

namespace PK::Rendering::Passes
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Math;
    using namespace Utilities;
    using namespace Structs;
    using namespace Objects;

    PassPostEffectsComposite::PassPostEffectsComposite(AssetDatabase* assetDatabase, const ApplicationConfig* config)
    {
        m_computeComposite = assetDatabase->Find<Shader>("CS_PostEffectsComposite");

        auto hash = HashCache::Get();

        m_constantsPostProcess = CreateRef<ConstantBuffer>(BufferLayout(
            {
                {ElementType::Float4, hash->pk_CC_WhiteBalance },
                {ElementType::Float4, hash->pk_CC_Lift },
                {ElementType::Float4, hash->pk_CC_Gamma },
                {ElementType::Float4, hash->pk_CC_Gain },
                {ElementType::Float4, hash->pk_CC_HSV },
                {ElementType::Float4, hash->pk_CC_MixRed },
                {ElementType::Float4, hash->pk_CC_MixGreen },
                {ElementType::Float4, hash->pk_CC_MixBlue },

                {ElementType::Float, hash->pk_CC_LumaContrast },
                {ElementType::Float, hash->pk_CC_LumaGain },
                {ElementType::Float, hash->pk_CC_LumaGamma },
                {ElementType::Float, hash->pk_CC_Vibrance },
                {ElementType::Float, hash->pk_CC_Contribution },

                {ElementType::Float, hash->pk_Vignette_Intensity },
                {ElementType::Float, hash->pk_Vignette_Power },

                {ElementType::Float, hash->pk_FilmGrain_Luminance },
                {ElementType::Float, hash->pk_FilmGrain_Intensity },

                {ElementType::Float, hash->pk_AutoExposure_MinLogLuma },
                {ElementType::Float, hash->pk_AutoExposure_InvLogLumaRange },
                {ElementType::Float, hash->pk_AutoExposure_LogLumaRange },
                {ElementType::Float, hash->pk_AutoExposure_Target },
                {ElementType::Float, hash->pk_AutoExposure_Speed },

                {ElementType::Float, hash->pk_Bloom_Intensity },
                {ElementType::Float, hash->pk_Bloom_DirtIntensity },

                {ElementType::Float, hash->pk_TAA_Sharpness },
                {ElementType::Float, hash->pk_TAA_BlendingStatic },
                {ElementType::Float, hash->pk_TAA_BlendingMotion },
                {ElementType::Float, hash->pk_TAA_MotionAmplification },
            }), 
            "Constants.PostProcess");

        GraphicsAPI::SetBuffer(hash->pk_PostEffectsParams, *m_constantsPostProcess.get());
    }

    void PassPostEffectsComposite::Render(CommandBuffer* cmd, RenderTexture* destination)
    {
        auto hash = HashCache::Get();
        auto color = destination->GetColor(0);
        auto resolution = destination->GetResolution();

        cmd->BeginDebugScope("PostEffects.Composite", PK_COLOR_YELLOW);
        GraphicsAPI::SetImage(hash->pk_Image, color, 0, 0);
        cmd->Dispatch(m_computeComposite, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();
    }

    void PassPostEffectsComposite::OnUpdateParameters(AssetImportToken<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto config = token->asset;

        m_bloomLensDirtTexture = token->assetDatabase->Load<Texture>(config->FileBloomDirt.value.c_str());
        m_lut = token->assetDatabase->Load<Texture>("res/textures/T_CC_LUT32.ktx2");

        auto smp = m_lut->GetSamplerDescriptor();
        smp.wrap[0] = WrapMode::Clamp;
        smp.wrap[1] = WrapMode::Clamp;
        smp.wrap[2] = WrapMode::Clamp;
        smp.filterMin = FilterMode::Trilinear;
        smp.filterMag = FilterMode::Trilinear;
        m_lut->SetSampler(smp);

        GraphicsAPI::SetTexture(hash->pk_Bloom_LensDirtTex, m_bloomLensDirtTexture);
        GraphicsAPI::SetTexture(hash->pk_CC_LutTex, m_lut);

        m_constantsPostProcess->Set<float>(hash->pk_Bloom_Intensity, glm::exp(config->BloomIntensity) - 1.0f);
        m_constantsPostProcess->Set<float>(hash->pk_Bloom_DirtIntensity, glm::exp(config->BloomLensDirtIntensity) - 1.0f);

        m_constantsPostProcess->Set<float>(hash->pk_AutoExposure_MinLogLuma, config->AutoExposureLuminanceMin);
        m_constantsPostProcess->Set<float>(hash->pk_AutoExposure_InvLogLumaRange, 1.0f / config->AutoExposureLuminanceRange);
        m_constantsPostProcess->Set<float>(hash->pk_AutoExposure_LogLumaRange, config->AutoExposureLuminanceRange);
        m_constantsPostProcess->Set<float>(hash->pk_AutoExposure_Target, config->AutoExposureTarget);
        m_constantsPostProcess->Set<float>(hash->pk_AutoExposure_Speed, config->AutoExposureSpeed);

        m_constantsPostProcess->Set<float>(hash->pk_TAA_Sharpness, config->TAASharpness);
        m_constantsPostProcess->Set<float>(hash->pk_TAA_BlendingStatic, config->TAABlendingStatic);
        m_constantsPostProcess->Set<float>(hash->pk_TAA_BlendingMotion, config->TAABlendingMotion);
        m_constantsPostProcess->Set<float>(hash->pk_TAA_MotionAmplification, config->TAAMotionAmplification);

        m_constantsPostProcess->Set<float>(hash->pk_Vignette_Intensity, config->VignetteIntensity);
        m_constantsPostProcess->Set<float>(hash->pk_Vignette_Power, config->VignettePower);

        m_constantsPostProcess->Set<float>(hash->pk_FilmGrain_Luminance, config->FilmGrainLuminance);
        m_constantsPostProcess->Set<float>(hash->pk_FilmGrain_Intensity, config->FilmGrainIntensity);

        color lift, gamma, gain;
        Functions::GenerateLiftGammaGain(Functions::HexToRGB(config->CC_Shadows), Functions::HexToRGB(config->CC_Midtones), Functions::HexToRGB(config->CC_Highlights), &lift, &gamma, &gain);
        m_constantsPostProcess->Set<float4>(hash->pk_CC_WhiteBalance, Functions::GetWhiteBalance(config->CC_TemperatureShift, config->CC_Tint));
        m_constantsPostProcess->Set<float4>(hash->pk_CC_Lift, lift);
        m_constantsPostProcess->Set<float4>(hash->pk_CC_Gamma, gamma);
        m_constantsPostProcess->Set<float4>(hash->pk_CC_Gain, gain);
        m_constantsPostProcess->Set<float4>(hash->pk_CC_HSV, float4(config->CC_Hue, config->CC_Saturation, config->CC_Value, 1.0f));
        m_constantsPostProcess->Set<float4>(hash->pk_CC_MixRed, Functions::HexToRGB(config->CC_ChannelMixerRed));
        m_constantsPostProcess->Set<float4>(hash->pk_CC_MixGreen, Functions::HexToRGB(config->CC_ChannelMixerGreen));
        m_constantsPostProcess->Set<float4>(hash->pk_CC_MixBlue, Functions::HexToRGB(config->CC_ChannelMixerBlue));
        m_constantsPostProcess->Set<float>(hash->pk_CC_LumaContrast, config->CC_Contrast);
        m_constantsPostProcess->Set<float>(hash->pk_CC_LumaGain, config->CC_Gain);
        m_constantsPostProcess->Set<float>(hash->pk_CC_LumaGamma, 1.0f / config->CC_Gamma);
        m_constantsPostProcess->Set<float>(hash->pk_CC_Vibrance, config->CC_Vibrance);
        m_constantsPostProcess->Set<float>(hash->pk_CC_Contribution, config->CC_Contribution);
        m_constantsPostProcess->FlushBuffer(QueueType::Transfer);
    }
}