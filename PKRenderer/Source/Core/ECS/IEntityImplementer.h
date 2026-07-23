#pragma once
#include "IEntityImplementerBucket.h"

namespace PK
{
    struct IEntityImplementer
    {
        ImplementerBucket* bucket = nullptr;
        uint32_t referenceCount = 0u;
        uint32_t index = 0u;

        virtual ~IEntityImplementer() = default;
    };
}
