#pragma once
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/Texture.h"

namespace PK::Rendering::RHI
{
    struct BuiltInResources
    {
        Objects::BufferRef AtomicCounter;
        Objects::TextureRef WhiteTexture2D;
        Objects::TextureRef BlackTexture2D;
        Objects::TextureRef TransparentTexture2D;
        BuiltInResources();
    };
}