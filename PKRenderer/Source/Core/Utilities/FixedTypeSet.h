#pragma once
#include <type_traits>
#include <Core/Utilities/FixedUnique.h>

namespace PK
{
    template<typename... Args>
    struct FixedTypeSet;
    
    template<>
    struct FixedTypeSet<>
    {
        template<typename TBase>
        constexpr TBase* GetInstance()
        {
            return nullptr;
        }
    };
    
    template <typename T, typename ... TRest>
    struct FixedTypeSet<T, TRest...> : private FixedTypeSet<TRest...>
    {
        template<typename TBase>
        constexpr TBase* GetInstance()
        {
            if constexpr (std::is_base_of<TBase, T>())
            {
                return static_cast<TBase*>(&m_instance);
            }
    
            return FixedTypeSet<TRest...>::template GetInstance<TBase>();
        }
    
        T m_instance;
    };

    template<typename... Args>
    struct FixedUniqueSet;

    template<>
    struct FixedUniqueSet<>
    {
        template<typename TType>
        constexpr FixedUnique<TType>* GetInstance()
        {
            return nullptr;
        }
    };

    template <typename T, typename ... TRest>
    struct FixedUniqueSet<T, TRest...> : private FixedUniqueSet<TRest...>
    {
        template<typename TType>
        constexpr FixedUnique<TType>* GetInstance()
        {
            if constexpr (std::is_same<TType, T>())
            {
                return &m_instance;
            }

            return FixedUniqueSet<TRest...>::template GetInstance<TType>();
        }

        FixedUnique<T> m_instance;
    };
}
