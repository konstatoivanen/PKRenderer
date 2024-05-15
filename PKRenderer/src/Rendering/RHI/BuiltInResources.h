#pragma once
#include "Rendering/RHI/RHI.h"

namespace PK::Rendering::RHI
{
    struct BuiltInResources
    {
        RHI::Objects::BufferRef AtomicCounter;
        RHI::Objects::TextureRef WhiteTexture2D;
        RHI::Objects::TextureRef BlackTexture2D;
        RHI::Objects::TextureRef TransparentTexture2D;
        BuiltInResources();
    };
}
