#include "PrecompiledHeader.h"
#include "Core/Platform/Platform.h"
#include "Ref.h"

namespace PK
{
    void SharedObjectBase::IncrementWeakRef() noexcept { Platform::InterlockedIncrement(reinterpret_cast<volatile uint32_t*>(&weakCount)); }
    void SharedObjectBase::IncrementStrongRef() noexcept { Platform::InterlockedIncrement(reinterpret_cast<volatile uint32_t*>(&referenceCount)); }
    void SharedObjectBase::DecrementWeakRef() noexcept { if (Platform::InterlockedDecrement(reinterpret_cast<volatile uint32_t*>(&weakCount)) == 0) Delete(); }

    void SharedObjectBase::DecrementStrongRef() noexcept
    {
        if (Platform::InterlockedDecrement(reinterpret_cast<volatile uint32_t*>(&referenceCount)) == 0)
        {
            Destroy();
            DecrementWeakRef();
        }
    }

    bool SharedObjectBase::IncrementRefNonZero() noexcept
    {
        volatile auto& volatileRefCount = referenceCount;
        auto count = Platform::AtomicRead(&volatileRefCount);

        while (count != 0u)
        {
            const auto oldValue = Platform::InterlockedCompareExchange(&volatileRefCount, count + 1u, count);

            if (oldValue == count)
            {
                return true;
            }

            count = oldValue;
        }

        return false;
    }
}
