#pragma once
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering
{
    struct GBuffersView
    {
        Objects::Texture* color;
        Objects::Texture* normals;
        Objects::Texture* zbias;
        Objects::Texture* depth;
    };

    struct GBuffersViewFull
    {
        GBuffersView current;
        GBuffersView previous;
    };

    struct GBuffers
    {
        Utilities::Ref<Objects::Texture> color;
        Utilities::Ref<Objects::Texture> normals;
        Utilities::Ref<Objects::Texture> zbias;
        Utilities::Ref<Objects::Texture> depth;
    };

    struct GBuffersFull
    {
        GBuffers current;
        GBuffers previous;

        inline GBuffersViewFull GetView()
        {
            return 
            { 
                { current.color.get(), current.normals.get(), current.zbias.get(), current.depth.get()}, 
                { previous.color.get(), previous.normals.get(), nullptr, previous.depth.get()}
            };
        }

        inline bool Validate(const Math::uint3 resolution)
        {
            auto value = false;
            value |= current.color->Validate(resolution);
            value |= current.depth->Validate(resolution);
            value |= current.normals->Validate(resolution);
            value |= current.zbias->Validate(resolution);
            value |= previous.color->Validate(resolution);
            value |= previous.normals->Validate(resolution);
            value |= previous.depth->Validate(resolution);
            return value;
        }
    };

}