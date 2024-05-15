#pragma once
#include "Utilities/PropertyBlock.h"
#include "Rendering/RHI/Layout.h"

namespace PK::Rendering::Objects
{
    class ShaderPropertyBlock : public Utilities::PropertyBlock
    {
        public:
            ShaderPropertyBlock(uint64_t capacityBytes, uint64_t capacityProperties) : Utilities::PropertyBlock(capacityBytes, capacityProperties) {}
            virtual ~ShaderPropertyBlock() = 0;
            void ReserveLayout(const RHI::BufferLayout& layout);
    };
}