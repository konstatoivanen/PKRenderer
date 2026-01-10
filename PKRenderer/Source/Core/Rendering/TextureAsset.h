#pragma once
#include "Core/Math/MathFwd.h"
#include "Core/Assets/Asset.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct TextureAsset : public Asset
    {
        TextureAsset(const char* filepath);

        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;
    
        operator RHITexture* ();
        operator const RHITexture* () const;
        
    private: 
        RHITextureRef m_texture = nullptr;
    };
}
