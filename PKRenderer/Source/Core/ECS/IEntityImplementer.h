#pragma once
#include <stdint.h>

namespace PK
{
    struct IEntityImplementer
    {
        struct ImplementerBucket* bucket = nullptr;
        uint32_t referenceCount = 0u;
        uint32_t index = 0u;

        virtual ~IEntityImplementer() = default;
    };
}
