#pragma once
#include "Core/Base/Reflect.h"

namespace PK
{
    template <typename TEntityStruct>
    concept TIsValidEntityStruct = requires
    {
        &TEntityStruct::entityId;
        requires TIsSame<TRemoveCVRef_T<decltype(TEntityStruct::entityId)>, uint32_t*>;
        requires TIsClass<TEntityStruct>&& TIsAggregate<TEntityStruct>&& TIsStandardLayout<TEntityStruct>;
    };

    template <typename TEntity, typename TDescriptor>
    concept TEntityHasOnCreate = requires(struct EntityDatabase* db, TEntity& entity, const TDescriptor& desc)
    {
        TEntity::OnCreate(db, entity, desc);
    };

    template<typename TTuple>
    struct TMakeEntityComposition;

    template<typename...Args>
    struct TMakeEntityComposition<Tuple<Args...>>
    {
        constexpr static const size_t N = sizeof...(Args);
        static_assert(N > 0ull, "Cannot make a sorted tuple with zero elements");
        
        struct HashData
        {
            Pair<UUID128, size_t> hashes[N];
            size_t count;
        };

        static constexpr auto Filtered = []() 
        {
            size_t idx = 0;
            HashData result {{ Pair<UUID128, size_t>{pk_type_uuid128<Args>, idx++}... },1u};

            for (auto i = 0ull; i < N - 1ull; ++i) 
            for (auto j = 0ull; j < N - i - 1ull; ++j) 
            {
                if (result.hashes[j + 1ull].first < result.hashes[j].first)
                {
                    PK::Swap(result.hashes[j], result.hashes[j + 1ull]);
                }
            }
            
            for (auto i = 1ull; i < N; ++i) 
            {
                if (!(result.hashes[i].first == result.hashes[result.count - 1ull].first))
                {
                    result.hashes[result.count++] = result.hashes[i];
                }
            }

            return result;
        }();

        using Type = decltype([]<size_t...I>(TIndexSequence<I...>) 
        {
            return Tuple<Sequence::TypeAt<Filtered.hashes[I].second,Tuple<Args...>>...>{};
        }
        (TMakeIndexSequence<Filtered.count>{}));
    };

    template<typename T>
    using TStructToEntityComposition = typename TMakeEntityComposition<Sequence::OnlyPtr<TReflectTypes<T>>>::Type;

    template<typename T>
    constexpr uint64_t pk_entity_composition_uuid() { return pk_type_uuid64<TStructToEntityComposition<T>>; }
}
