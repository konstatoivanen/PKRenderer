#pragma once
#include "Core/Math/MathFwd.h"
#include "Core/Assets/Asset.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct TextureAsset : public AssetWithImport<>
    {
        TextureAsset() {};

        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;
    
        operator RHITexture* ();
        operator const RHITexture* () const;

        void AssetImport(const char* filepath) final;
        
    private: 
        RHITextureRef m_texture = nullptr;
    };
}