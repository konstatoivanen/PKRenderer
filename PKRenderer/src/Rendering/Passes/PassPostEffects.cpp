#include "PrecompiledHeader.h"
#include "PassPostEffects.h"
#include "Rendering/HashCache.h"

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
        m_bloomLensDirtTexture = assetDatabase->Load<Texture>(config->FileBloomDirt.value.c_str());
    }
    
    void PassPostEffectsComposite::Render(CommandBuffer* cmd, RenderTexture* destination)
    {
        auto hash = HashCache::Get();
        auto color = destination->GetColor(0);
        auto resolution = destination->GetResolution();

        cmd->BeginDebugScope("PostEffects.Composite", PK_COLOR_YELLOW);
        cmd->SetImage(hash->_MainTex, color, 0, 0);
        cmd->SetTexture(hash->pk_BloomLensDirtTex, m_bloomLensDirtTexture);
        cmd->Dispatch(m_computeComposite, { (uint)glm::ceil(resolution.x / 16.0f), (uint)glm::ceil(resolution.y / 4.0f), 1u });
        cmd->EndDebugScope();
    }
}