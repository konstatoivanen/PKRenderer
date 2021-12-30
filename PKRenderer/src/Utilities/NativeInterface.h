#pragma once

namespace PK::Utilities
{
    template<typename Base>
    struct NativeInterface
    {
        template<typename Child>
        Child* GetNative()
        {
            static_assert(std::is_base_of<Base, Child>::value, "Child doesn't derive from base!");
            return static_cast<Child*>(this);
        }

        template<typename Child>
        const Child* GetNative() const
        {
            static_assert(std::is_base_of<Base, Child>::value, "Child doesn't derive from base!");
            return static_cast<const Child*>(this);
        }
    };
}