#pragma once
#include "Core/Assets/Asset.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    struct TextureAsset : public Core::Assets::AssetWithImport<>
    {
        void AssetImport(const char* filepath) final;
        Texture* GetRHI();
        const Texture* GetRHI() const;
        operator Texture* ();
        operator const Texture* () const;
        private: TextureRef m_texture;
    };
}