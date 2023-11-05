#pragma once
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering
{
    struct GBuffersView
    {
        RHI::Objects::Texture* color;
        RHI::Objects::Texture* normals;
        RHI::Objects::Texture* depthBiased;
        RHI::Objects::Texture* depth;
    };

    struct GBuffersViewFull
    {
        GBuffersView current;
        GBuffersView previous;
    };

    struct GBuffers
    {
        RHI::Objects::TextureRef color;
        RHI::Objects::TextureRef normals;
        RHI::Objects::TextureRef depthBiased;
        RHI::Objects::TextureRef depth;
    };

    struct GBuffersFull
    {
        GBuffers current;
        GBuffers previous;

        inline GBuffersViewFull GetView()
        {
            return 
            { 
                { current.color.get(), current.normals.get(), current.depthBiased.get(), current.depth.get()},
                { previous.color.get(), previous.normals.get(), previous.depthBiased.get(), previous.depth.get()}
            };
        }

        inline bool Validate(const Math::uint3 resolution)
        {
            auto value = false;
            value |= current.color->Validate(resolution);
            value |= current.depth->Validate(resolution);
            value |= current.normals->Validate(resolution);
            value |= current.depthBiased->Validate(resolution);
            value |= previous.color->Validate(resolution);
            value |= previous.normals->Validate(resolution);
            value |= previous.depth->Validate(resolution);
            value |= previous.depthBiased->Validate(resolution);
            return value;
        }
    };

}