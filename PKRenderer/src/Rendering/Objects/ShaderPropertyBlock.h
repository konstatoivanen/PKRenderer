#pragma once
#include "Utilities/PropertyBlock.h"
#include "Rendering/Structs/Layout.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class ShaderPropertyBlock : public PropertyBlock
    {
        public:
            ShaderPropertyBlock(uint64_t capacity) : PropertyBlock(capacity) {}
            ShaderPropertyBlock(void* foreignBuffer, uint64_t capacity) : PropertyBlock(foreignBuffer, capacity) {}
            void ReserveLayout(const BufferLayout& layout);
    };
}