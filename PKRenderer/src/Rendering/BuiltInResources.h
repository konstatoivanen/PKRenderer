#pragma once
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering
{
    struct BuiltInResources
    {
        PK::Utilities::Ref<PK::Rendering::Objects::Buffer> AtomicCounter;
        PK::Utilities::Ref<PK::Rendering::Objects::Texture> WhiteTexture2D;
        PK::Utilities::Ref<PK::Rendering::Objects::Texture> BlackTexture2D;
        PK::Utilities::Ref<PK::Rendering::Objects::Texture> TransparentTexture2D;
        BuiltInResources();
    };
}