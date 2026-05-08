#pragma once
#include "Ref.h"
#include "TypeIndex.h"

namespace PK
{
    template<typename... Args>
    struct FixedTypeSet;
    
    template<>
    struct FixedTypeSet<>
    {
        template<typename TType>
        constexpr TType* GetInstance()
        {
            return nullptr;
        }

        template<typename TType>
        inline TType* GetInstance([[maybe_unused]] uint32_t typeIndex)
        {
            return nullptr;
        }
    };
    
    template <typename T, typename ... TRest>
    struct FixedTypeSet<T, TRest...> : private FixedTypeSet<TRest...>
    {
        template<typename TType>
        constexpr TType* GetInstance()
        {
            if constexpr (TIsConvertible<T*, TType*>)
            {
                return static_cast<TType*>(&m_instance);
            }
            else
            {
                return FixedTypeSet<TRest...>::template GetInstance<TType>();
            }
        }

        template<typename TType>
        inline TType* GetInstance(uint32_t typeIndex)
        {
            // Note this is not fast but in most use cases only yields a short recursion.
            if (TIsConvertible<T*, TType*> && pk_type_index<T> == typeIndex)
            {
                return static_cast<TType*>(&m_instance);
            }
            else
            {
                return FixedTypeSet<TRest...>::template GetInstance<TType>(typeIndex);
            }
        }
    
        T m_instance;
    };


    template<typename TBase>
    struct IDerivedTypeSet
    {
        virtual TBase* GetInstance(uint32_t typeIndex) = 0;

        template<typename T>
        T* GetInstance() { return static_cast<T*>(GetInstance(pk_type_index<T>)); }
    };

    template<typename TBase, typename ... TTypes>
    struct DerivedTypeSet : public IDerivedTypeSet<TBase>, private FixedTypeSet<TTypes...>
    {
        TBase* GetInstance(uint32_t typeIndex) final
        {
            return FixedTypeSet<TTypes...>::template GetInstance<TBase>(typeIndex);
        }
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
            if constexpr (TIsSame<TType, T>)
            {
                return &m_instance;
            }

            return FixedUniqueSet<TRest...>::template GetInstance<TType>();
        }

        FixedUnique<T> m_instance;
    };
}
