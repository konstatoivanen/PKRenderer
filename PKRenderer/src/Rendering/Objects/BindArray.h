#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;

    template<typename T>
    class BindArray : public NoCopy, public NativeInterface<BindArray<T>>
    {
        public:
            static Ref<BindArray<T>> Create(size_t capacity);
            virtual int32_t Add(T* value, void* bindInfo) = 0;
            virtual int32_t Add(T* value) = 0;
            virtual int32_t Add(const T* value) = 0;
            virtual void Clear() = 0;
    };
}