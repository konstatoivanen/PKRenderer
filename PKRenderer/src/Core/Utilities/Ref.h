#pragma once
#include <memory>

#define PK_STACK_ALLOC(Type, count) reinterpret_cast<Type*>(alloca(sizeof(Type) * count))

#define PK_CONTIGUOUS_ALLOC(Type, count) reinterpret_cast<Type*>(calloc(count, sizeof(Type)));

namespace PK
{
    template<typename T>
    using Scope = std::unique_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Scope<T> CreateScope(Args&& ... args)
    {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    using Ref = std::shared_ptr<T>;
    template<typename T, typename ... Args>
    constexpr Ref<T> CreateRef(Args&& ... args)
    {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    constexpr bool IsEqualRef(const Ref<T>& a, const Ref<T>& b)
    {
        return a == b || (a && b && *a == *b);
    }

    template<typename T>
    using Weak = std::weak_ptr<T>;
}