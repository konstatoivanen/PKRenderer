#include "PrecompiledHeader.h"
#include "Rendering/HashCache.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "PassEnvBackground.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    PassEnvBackground::PassEnvBackground(AssetDatabase* assetDatabase)
    {
        auto hash = HashCache::Get();
        m_backgroundShader = assetDatabase->Find<Shader>("SH_VS_EnvBackground");
        m_integrateSHShader = assetDatabase->Find<Shader>("CS_IntegrateEnvSH");
        m_shBuffer = Buffer::Create(ElementType::Float4, 4, BufferUsage::DefaultStorage, "Scene.Env.SHBuffer");
        GraphicsAPI::SetBuffer(hash->pk_SceneEnv_SH, m_shBuffer.get());
    }

    void PassEnvBackground::ComputeSH(CommandBuffer* cmd)
    {
        cmd->Dispatch(m_integrateSHShader, 0, { 1u, 1u, 1u });
    }

    void PassEnvBackground::RenderBackground(CommandBuffer* cmd)
    {
        cmd->Blit(m_backgroundShader);
    }

    void PassEnvBackground::OnUpdateParameters(AssetImportToken<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto tex = token->assetDatabase->Load<Texture>(token->asset->FileBackgroundTexture.value.c_str());
        auto sampler = tex->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Mirror;
        sampler.wrap[1] = WrapMode::Mirror;
        sampler.wrap[2] = WrapMode::Mirror;
        tex->SetSampler(sampler);
        GraphicsAPI::SetTexture(hash->pk_SceneEnv, tex);
    }
}
