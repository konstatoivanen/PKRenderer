#include "PrecompiledHeader.h"
#include "Core/Assets/AssetDatabase.h"
#include "Core/ApplicationConfig.h"
#include "Graphics/RHI/RHIBuffer.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Graphics/Shader.h"
#include "Graphics/TextureAsset.h"
#include "Renderer/HashCache.h"
#include "PassEnvBackground.h"

namespace PK::Renderer::Passes
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core;
    using namespace PK::Core::Assets;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

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

    void PassEnvBackground::ComputeSH(CommandBufferExt cmd)
    {
        cmd.Dispatch(m_integrateSHShader, 0, { 1u, 1u, 1u });
    }

    void PassEnvBackground::RenderBackground(CommandBufferExt cmd)
    {
        cmd.Blit(m_backgroundShader);
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
