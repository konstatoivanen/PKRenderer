#pragma once
#include "Utilities/Ref.h"
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"

namespace PK::Graphics::RHI
{
    template<typename T>
    struct RHIBindArray : public Utilities::NoCopy, public Utilities::NativeInterface<RHIBindArray<T>>
    {
        virtual ~RHIBindArray() = 0;
        virtual int32_t Add(T* value, void* bindInfo) = 0;
        virtual int32_t Add(T* value) = 0;
        virtual void Clear() = 0;
    };

    template<typename T>
    using RHIBindArrayRef = PK::Utilities::Ref<RHIBindArray<T>>;
}