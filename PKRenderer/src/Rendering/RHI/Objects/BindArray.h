#pragma once
#include "Utilities/Ref.h"
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"

namespace PK::Rendering::RHI::Objects
{
    template<typename T>
    struct BindArray : public Utilities::NoCopy, public Utilities::NativeInterface<BindArray<T>>
    {
        virtual ~BindArray() = 0;
        virtual int32_t Add(T* value, void* bindInfo) = 0;
        virtual int32_t Add(T* value) = 0;
        virtual void Clear() = 0;
    };

    template<typename T>
    using BindArrayRef = PK::Utilities::Ref<BindArray<T>>;
}