#pragma once
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics::RHI
{
    struct BuiltInResources
    {
        RHI::RHIBufferRef AtomicCounter;
        RHI::RHITextureRef WhiteTexture2D;
        RHI::RHITextureRef BlackTexture2D;
        RHI::RHITextureRef TransparentTexture2D;
        BuiltInResources();
    };
}
