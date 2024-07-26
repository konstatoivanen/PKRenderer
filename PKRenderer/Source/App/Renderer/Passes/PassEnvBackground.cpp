#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/CLI/CVariableRegister.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Rendering/ConstantBuffer.h"
#include "App/Renderer/HashCache.h"
#include "App/Renderer/RenderView.h"
#include "App/Renderer/RenderViewSettings.h"
#include "PassEnvBackground.h"

namespace PK::App
{
    PassEnvBackground::PassEnvBackground(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassEnvBackground.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        auto hash = HashCache::Get();
        m_backgroundShader = assetDatabase->Find<ShaderAsset>("VS_EnvBackground");
        m_integrateSHShader = assetDatabase->Find<ShaderAsset>("CS_IntegrateEnvSH");
        m_shBuffer = RHI::CreateBuffer<float4>(4ull, BufferUsage::DefaultStorage, "Scene.Env.SHBuffer");
        RHI::SetBuffer(hash->pk_SceneEnv_SH, m_shBuffer.get());
    }

    void PassEnvBackground::SetViewConstants(RenderView* view)
    {
        auto hash = HashCache::Get();
        auto& settings = view->settings.EnvBackgroundSettings;
        view->constants->Set<float>(hash->pk_SceneEnv_Exposure, settings.Exposure);

        auto texture = settings.EnvironmentTextureAsset != nullptr ? 
            settings.EnvironmentTextureAsset->GetRHI() : 
            RHI::GetBuiltInResources()->WhiteTexture2D.get();
        
        if (texture != m_backgroundTexture)
        {
            m_backgroundTexture = texture;
            auto sampler = texture->GetSamplerDescriptor();
            sampler.wrap[0] = WrapMode::Mirror;
            sampler.wrap[1] = WrapMode::Mirror;
            sampler.wrap[2] = WrapMode::Mirror;
            texture->SetSampler(sampler);
            RHI::SetTexture(hash->pk_SceneEnv, texture);
        }
    }

    void PassEnvBackground::ComputeSH(CommandBufferExt cmd)
    {
        cmd.Dispatch(m_integrateSHShader, 0, { 1u, 1u, 1u });
    }

    void PassEnvBackground::RenderBackground(CommandBufferExt cmd)
    {
        cmd.Blit(m_backgroundShader);
    }
}
