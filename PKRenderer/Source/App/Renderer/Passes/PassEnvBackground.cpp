#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "App/Renderer/HashCache.h"
#include "App/RendererConfig.h"
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

    void PassEnvBackground::ComputeSH(CommandBufferExt cmd)
    {
        cmd.Dispatch(m_integrateSHShader, 0, { 1u, 1u, 1u });
    }

    void PassEnvBackground::RenderBackground(CommandBufferExt cmd)
    {
        cmd.Blit(m_backgroundShader);
    }

    void PassEnvBackground::OnUpdateParameters(AssetImportEvent<RendererConfig>* token)
    {
        auto hash = HashCache::Get();
        auto tex = token->assetDatabase->Load<TextureAsset>(token->asset->FileBackgroundTexture)->GetRHI();
        auto sampler = tex->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Mirror;
        sampler.wrap[1] = WrapMode::Mirror;
        sampler.wrap[2] = WrapMode::Mirror;
        tex->SetSampler(sampler);
        RHI::SetTexture(hash->pk_SceneEnv, tex);
    }
}
