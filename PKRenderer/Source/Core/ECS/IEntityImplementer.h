#pragma once
#include "Core/Utilities/Tuple.h"
#include "IEntityImplementerBucket.h"

namespace PK
{
    struct IEntityImplementer
    {
        ImplementerBucket* bucket = nullptr;
        uint32_t referenceCount = 0u;
        uint32_t index = 0u;
    };

    template <typename... Args>
    struct TEntityImplementer : public IEntityImplementer, public Args...
    {
        using TComponents = Tuple<Args...>;
    };
}
