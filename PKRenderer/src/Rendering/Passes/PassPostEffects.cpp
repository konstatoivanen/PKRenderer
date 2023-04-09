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
        GraphicsAPI::SetTexture(HashCache::Get()->pk_BloomLensDirtTex, m_bloomLensDirtTexture);
    }

    void PassPostEffectsComposite::Render(CommandBuffer* cmd, RenderTexture* destination)
    {
        auto hash = HashCache::Get();
        auto color = destination->GetColor(0);
        auto resolution = destination->GetResolution();

        cmd->BeginDebugScope("PostEffects.Composite", PK_COLOR_YELLOW);
        GraphicsAPI::SetImage(hash->_MainTex, color, 0, 0);
        cmd->Dispatch(m_computeComposite, { resolution.x, resolution.y, 1u });
        cmd->EndDebugScope();
    }
}