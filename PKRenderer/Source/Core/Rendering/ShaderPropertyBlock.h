#pragma once
#include "Core/Utilities/PropertyBlock.h"
#include "Core/RHI/Layout.h"

namespace PK
{
    struct ShaderPropertyBlock : public PropertyBlock
    {
        ShaderPropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties) : PropertyBlock(capacityBytes, capacityProperties) {}
        virtual ~ShaderPropertyBlock() = 0;
        void ReserveLayout(const ShaderStructLayout& layout);
    };
}
