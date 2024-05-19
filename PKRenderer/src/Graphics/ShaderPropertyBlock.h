#pragma once
#include "Utilities/PropertyBlock.h"
#include "Graphics/RHI/Layout.h"

namespace PK::Graphics
{
    struct ShaderPropertyBlock : public Utilities::PropertyBlock
    {
        ShaderPropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties) : Utilities::PropertyBlock(capacityBytes, capacityProperties) {}
        virtual ~ShaderPropertyBlock() = 0;
        void ReserveLayout(const RHI::BufferLayout& layout);
    };
}