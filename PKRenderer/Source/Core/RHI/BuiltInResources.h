#pragma once
#include "Core/RHI/RHI.h"

namespace PK
{
    struct BuiltInResources
    {
        RHIBufferRef AtomicCounter;
        RHITextureRef WhiteTexture2D;
        RHITextureRef ErrorTexture2D;
        RHITextureRef BlackTexture2D;
        RHITextureRef BlackTexture2DArray;
        RHITextureRef TransparentTexture2D;
        BuiltInResources();
    };
}
