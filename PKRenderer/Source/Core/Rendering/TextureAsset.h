#pragma once
#include "Core/Assets/Asset.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct TextureAsset : public AssetWithImport<>
    {
        void AssetImport(const char* filepath) final;
        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;
        operator RHITexture* ();
        operator const RHITexture* () const;
        private: RHITextureRef m_texture;
    };
}