#pragma once
#include "Utilities/PropertyBlock.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    class ShaderPropertyBlock : public Utilities::PropertyBlock
    {
        public:
            ShaderPropertyBlock(uint64_t capacity) : Utilities::PropertyBlock(capacity) {}
            ShaderPropertyBlock(void* foreignBuffer, uint64_t capacity) : Utilities::PropertyBlock(foreignBuffer, capacity) {}
            void ReserveLayout(const RHI::BufferLayout& layout);
    };
}