#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/ApplicationConfig.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/Texture.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/Objects/TextureAsset.h"
#include "Rendering/HashCache.h"
#include "PassEnvBackground.h"

namespace PK::Rendering::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    PassEnvBackground::PassEnvBackground(AssetDatabase* assetDatabase)
    {
        PK_LOG_VERBOSE("PassEnvBackground.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        auto hash = HashCache::Get();
        m_backgroundShader = assetDatabase->Find<Shader>("VS_EnvBackground");
        m_integrateSHShader = assetDatabase->Find<Shader>("CS_IntegrateEnvSH");
        m_shBuffer = RHICreateBuffer<float4>(4ull, BufferUsage::DefaultStorage, "Scene.Env.SHBuffer");
        RHISetBuffer(hash->pk_SceneEnv_SH, m_shBuffer.get());
    }

    void PassEnvBackground::ComputeSH(CommandBuffer* cmd)
    {
        cmd->Dispatch(m_integrateSHShader, 0, { 1u, 1u, 1u });
    }

    void PassEnvBackground::RenderBackground(CommandBuffer* cmd)
    {
        cmd->Blit(m_backgroundShader);
    }

    void PassEnvBackground::OnUpdateParameters(AssetImportEvent<ApplicationConfig>* token)
    {
        auto hash = HashCache::Get();
        auto tex = token->assetDatabase->Load<TextureAsset>(token->asset->FileBackgroundTexture)->GetRHI();
        auto sampler = tex->GetSamplerDescriptor();
        sampler.wrap[0] = WrapMode::Mirror;
        sampler.wrap[1] = WrapMode::Mirror;
        sampler.wrap[2] = WrapMode::Mirror;
        tex->SetSampler(sampler);
        RHISetTexture(hash->pk_SceneEnv, tex);
    }
}
