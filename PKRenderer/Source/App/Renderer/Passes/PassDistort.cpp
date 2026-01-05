#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "App/Renderer/RenderPipelineBase.h"
#include "PassDistort.h"

namespace PK::App
{
    PassDistort::PassDistort(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE_FUNC("");
        m_computeDistort = assetDatabase->Find<ShaderAsset>("CS_Distort");
    }

    void PassDistort::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.DistortSettings;

        const auto distance = settings.PaniniProjectionAmount;
        const auto viewDist = 1.0f + distance;
        const auto aspect = (float)view->GetResolution().x / (float)view->GetResolution().y;
        const auto viewExtY = glm::tan(0.5f * view->fieldOfView);
        const auto viewExtX = aspect * viewExtY;
        const auto projHyp = glm::sqrt(viewExtX * viewExtX + 1.0f);
        const auto cylDistMinusD = 1.0f / projHyp;
        const auto cylDist = cylDistMinusD + distance;
        const auto cylPos = float2(viewExtX, viewExtY) * cylDistMinusD * (viewDist / cylDist);
        const auto scaleX = cylPos.x / viewExtX;
        const auto scaleY = cylPos.y / viewExtY;
        const auto scaleF = glm::min(scaleX, scaleY);
        const auto scale = glm::mix(1.0f, glm::clamp(scaleF, 0.0f, 1.0f), settings.PaniniProjectionScreenFit);
        const auto paniniParams = float4(viewExtX, viewExtY, distance, scale);

        view->constants->Set<float4>(hash->pk_Panini_Projection_Parameters, paniniParams);
        view->constants->Set<float>(hash->pk_Chromatic_Aberration_Amount, settings.ChromaticAberrationAmount);
        view->constants->Set<float>(hash->pk_Chromatic_Aberration_Power, settings.ChromaticAberrationPower);
    }

    void PassDistort::Render(CommandBufferExt cmd, RenderPipelineContext* ctx)
    {
        auto view = ctx->views[0];
        auto source = view->gbuffers.GetView().current.color;
        auto resources = view->GetResources<ViewResources>();
        auto resolution = source->GetResolution();
        auto& settings = view->settings.DistortSettings;

        if (glm::epsilonEqual(settings.PaniniProjectionAmount, 0.0f, 1e-4f) &&
            glm::epsilonEqual(settings.ChromaticAberrationAmount, 0.0f, 1e-4f))
        {
            resources->distortTexture = nullptr;
            return;
        }
        
        cmd->BeginDebugScope("Distort", PK_COLOR_MAGENTA);

        {
            TextureDescriptor descriptor{};
            descriptor.type = TextureType::Texture2D;
            descriptor.usage = TextureUsage::DefaultStorage;
            descriptor.format = TextureFormat::RGB9E5;
            descriptor.formatAlias = TextureFormat::R32UI;
            descriptor.layers = 1;
            descriptor.levels = 1;
            descriptor.resolution = resolution;
            descriptor.sampler.filterMin = FilterMode::Trilinear;
            descriptor.sampler.filterMag = FilterMode::Trilinear;
            descriptor.sampler.wrap[0] = view->settings.BloomSettings.BorderClip ? WrapMode::Border : WrapMode::Clamp;
            descriptor.sampler.wrap[1] = view->settings.BloomSettings.BorderClip ? WrapMode::Border : WrapMode::Clamp;
            descriptor.sampler.wrap[2] = view->settings.BloomSettings.BorderClip ? WrapMode::Border : WrapMode::Clamp;
            descriptor.sampler.borderColor = BorderColor::FloatBlack;
            RHI::ValidateTexture(resources->distortTexture, descriptor, "Distort.Texture");
        }

        auto distort = resources->distortTexture.get();
        auto hash = HashCache::Get();

        RHI::SetTexture(hash->pk_Texture, source, 0, 0);
        RHI::SetImage(hash->pk_Image, distort, 0, 0);
        cmd.Dispatch(m_computeDistort, 0, { resolution.x, resolution.y, 1u });
        
        RHI::SetTexture(hash->pk_Texture, distort, 0, 0);
        RHI::SetImage(hash->pk_Image, source, 0, 0);
        cmd.Dispatch(m_computeDistort, 1, { resolution.x, resolution.y, 1u });

        cmd->EndDebugScope();
    }
}
