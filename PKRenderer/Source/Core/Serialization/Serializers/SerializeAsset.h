#pragma once
#ifdef PK_SERIALIZE_HEADER
#include "Core/Serialization/Serialize.h"
#include "Core/Assets/AssetDatabase.h"

namespace PK
{
    template<typename T> 
    requires TIsPointer<T> && TIsBaseOf<PK::Asset, TRemovePtr_T<T>>
    struct ISerializer<T, void>
    {
        static void ReadVal(SerialNodeRead node, T* rhs)
        {
            auto pathsubstr = node.val();
            FixedString128 path(pathsubstr.len, pathsubstr.data());
            *rhs = AssetDatabase::Get()->Load<TRemovePtr_T<T>>(path).get();
        }

        static void WriteVal(SerialNodeWrite node, T const* rhs)
        {
            node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
        }
    };

    template<typename T>
    requires (!TIsPointer<T>) && TIsBaseOf<PK::Asset, T>
    struct ISerializer<Ref<T>, void>
    {
        static void ReadVal(SerialNodeRead node, Ref<T>* rhs)
        {
            auto pathsubstr = node.val();
            FixedString128 path(pathsubstr.len, pathsubstr.data());
            *rhs = AssetDatabase::Get()->Load<T>(path);
        }

        static void WriteVal(SerialNodeWrite node, Ref<T> const* rhs)
        {
            node << (*rhs)->GetFileName() |= ryml::VAL_DQUO;
        }
    };
}
#endif
