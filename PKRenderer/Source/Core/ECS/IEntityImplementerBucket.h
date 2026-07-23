#pragma once
#include "Core/Utilities/Memory.h"

namespace PK
{
    struct alignas(16) ImplementerBucket
    {
        constexpr static const uint32_t SIZE = 32768u;

        uint8_t data[SIZE];
        void (*destroyAt)(ImplementerBucket* bucket, uint32_t index);
        ImplementerBucket* previous;
        uint64_t capacity;
        uint64_t freeCount;

        ~ImplementerBucket()
        {
            Memory::Delete(previous);

            for (auto i = 0u; i < capacity; ++i)
            {
                destroyAt(this, i);
            }
        }
    };
}
