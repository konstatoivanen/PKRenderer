#pragma once
#include "Utilities/NativeInterface.h"
#include "Core/Assets/Asset.h"
#include "Rendering/RHI/Structs.h"
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::Objects
{
    typedef Utilities::Ref<struct TextureAsset> TextureAssetRef;

    struct TextureAsset : public Core::Assets::AssetWithImport<>
    {
        void AssetImport(const char* filepath) final;
        
        RHI::Objects::Texture* GetRHI();
        const RHI::Objects::Texture* GetRHI() const;
        operator RHI::Objects::Texture* ();
        operator const RHI::Objects::Texture* () const;
        
    private:
        RHI::Objects::TextureRef m_texture;
    };
}