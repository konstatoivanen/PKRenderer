#pragma once
#include <type_traits>

namespace PK
{
    template<typename... Args>
    struct FixedTypeSet;
    
    template<>
    struct FixedTypeSet<>
    {
        template<typename TBase>
        TBase* GetInstance()
        {
            return nullptr;
        }
    };
    
    template <typename T, typename ... TRest>
    struct FixedTypeSet<T, TRest...> : private FixedTypeSet<TRest...>
    {
        template<typename TBase>
        TBase* GetInstance()
        {
            if constexpr (std::is_base_of<TBase, T>())
            {
                return static_cast<TBase*>(&m_instance);
            }
    
            return FixedTypeSet<TRest...>::template GetInstance<TBase>();
        }
    
        T m_instance;
    };
}
