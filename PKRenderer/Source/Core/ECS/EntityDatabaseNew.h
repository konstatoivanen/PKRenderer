#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Types/UUID128.h"
#include "Core/Base/Types/Tuple.h"
#include "Core/Base/TypeMeta.h"
#include "Core/Base/Reflect.h"
#include "Core/Base/Hash.h"

namespace PK
{
    template<typename T>
    inline constexpr auto pk_ecs_type_uuid = []() constexpr noexcept
    {
        constexpr const auto typeName = pk_outer_type_name<T>;
        return Hash::MurmurHash128(typeName.str, typeName.length);
    }();

    template <typename TEntityStruct>
    concept TIsValidEntityStruct = requires
    {
        &TEntityStruct::entityId;
        requires TIsSame<TRemoveCVRef_T<decltype(TEntityStruct::entityId)>, uint32_t*>;
        requires TIsClass<TEntityStruct>&& TIsAggregate<TEntityStruct>&& TIsStandardLayout<TEntityStruct>;
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
            HashData result {{ Pair<UUID128, size_t>{pk_ecs_type_uuid<Args>, idx++}... },1u};

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



    // @TODO replace with UUID once we have a working hash for templated types.
    struct EntitiesDatabase
    {
        struct IComponent
        {
            const UUID128 typeUUID;
            const uint64_t stride;
            void (*const constructAt)(void* data, uint32_t index);
            void (*const removeAt)(void* data, uint32_t index, uint32_t last);
            void (*const clear)(void* data, uint32_t count);
            void (*const move)(void* dst, void* src, uint32_t count);

            template<typename T>
            constexpr static IComponent Get() noexcept
            {
                return
                {
                    pk_ecs_type_uuid<T>,
                    sizeof(T),
                    [](void* data, uint32_t index) { Memory::Construct(static_cast<T*>(data) + index);},
                    [](void* data, uint32_t index, uint32_t last) { static_cast<T*>(data)[index] = PK::MoveTemp(static_cast<T*>(data)[last]); },
                    [](void* data, uint32_t count) { Memory::ClearArray(static_cast<T*>(data), count); },
                    [](void* dst, void* src, uint32_t count) { Memory::MoveArray(static_cast<T*>(dst), static_cast<T*>(src), count); }
                };
            }
        };

        template <size_t N>
        struct IComponentArray
        {
            IComponent data[N];
        };

        template<typename T>
        inline static constexpr IComponentArray<T::Size> component_interfaces = []<size_t... I>(TIndexSequence<I...>)
        {
            return IComponentArray<T::Size>{{ IComponent::Get<typename Sequence::TypeAt<I, T>>()... }};
        }
        (TMakeIndexSequence<T::Size>{});

        struct Composition
        {
            const uint32_t componentStride;
            const uint32_t componentCount;
            const IComponent* components;
            uint32_t count;
            uint32_t capacity;
            void* buffer;
            void** streams;
        };


        struct Identifier
        {
            uint64_t identifier = 0ull;
            
            Identifier(uint32_t entityId, uint32_t entityIndex, uint32_t arrayIndex) 
            {
                identifier |= (uint64_t)entityId & 0xFFFFFFull;
                identifier |= ((uint64_t)entityIndex & 0xFFFFull) << 24ull;
                identifier |= ((uint64_t)arrayIndex & 0xFFFFFFull) << 48ull;
            }

            bool operator == (const Identifier& other) { return entityId() == other.entityId(); }
            bool operator != (const Identifier& other) { return entityId() != other.entityId(); }
            uint32_t entityId() const { return identifier & 0xFFFFFFull; }
            uint32_t entityIndex() const { return (identifier >> 24ull) & 0xFFFFull; }
            uint32_t arrayIndex() const { return (identifier >> 48ull) & 0xFFFFFFull; }
        };

        struct IdentifierHash 
        { 
            size_t operator()(const Identifier& k) const noexcept { return (size_t)k.entityId();}
        };


        template<typename TView>
        struct ViewIterator
        {
            struct Sentinel {};
            EntitiesDatabase* entityDb;
            Composition* viewdata;
            TView view;
            uint32_t groupIndex = 0u;
            uint32_t arrayCount = 0u;
            bool is_valid = false;

            constexpr ViewIterator(EntitiesDatabase* db, Composition* data) noexcept : entityDb(db), viewdata(data), is_valid(Next()) {}
            TView& operator*() { return view; }
            TView* operator->() { return &view; }
            const TView& operator*() const { return view; }
            const TView* operator->() const { return &view; }
            ViewIterator& operator++() { is_valid = Next(); return *this; }
            void operator++(int) { ++(*this); }
            friend bool operator!=(const ViewIterator& it, Sentinel) noexcept { return it.is_valid; }

            bool Next()
            {
                if (arrayCount)
                {
                    ReflectFields(view, [](auto& field)
                    {
                        if constexpr (TIsPointer<TRemoveCVRef_T<decltype(field)>>)
                        {
                            field++;
                        }
                    });
                    arrayCount--;
                    return true;
                }

                while (groupIndex < viewdata->count)
                {
                    auto index = static_cast<const uint32_t*>(viewdata->buffer)[groupIndex++];
                    auto composition = &entityDb->m_compositions[index].value;
                    
                    if (composition->count)
                    {
                        arrayCount = composition->count - 1ull;
                        view = entityDb->BindView<TView>(index, 0);
                        return true;
                    }
                }

                return false;
            }
        };

        template<typename TView>
        struct ViewRange
        {
            EntitiesDatabase* entityDb;
            Composition* viewdata;
            auto begin() const { return ViewIterator<TView>(entityDb, viewdata); }
            auto end() const { return typename ViewIterator<TView>::Sentinel{}; }
        };


        template<typename TEntity>
        void Reserve(size_t entryCount)
        {
            AllocateComposition<TEntity>(false, entryCount);
        }

        template<typename TEntityStruct> 
        TEntityStruct New()
        {
            auto entityIndex = AllocateComposition<TEntityStruct>(false, 1ull);
            auto identifier = NewEntity(entityIndex);
            return BindView<TEntityStruct>(entityIndex, identifier.arrayIndex());
        }

        template<typename TView> 
        ViewRange<TView> Query()
        {
            auto viewIndex = AllocateComposition<TView>(true, 0ull);
            return { this, &m_compositions[viewIndex].value };
        }

        template<typename TView>
        TView Query(uint32_t entityId)
        {
            auto* id = m_identifiers.GetValuePtr(Identifier(entityId, 0u, 0u));
            return id ? BindView<TView>(id->entityIndex(), id->arrayIndex()) : TView{};
        }

        template<typename TEntityStruct>
        requires TIsValidEntityStruct<TEntityStruct>
        void DeleteType() 
        {
            using TComposition = TStructToEntityComposition<TEntityStruct>;
            const auto typeIndex = pk_type_index<TComposition>;
            DeleteType(typeIndex);
        }

        void Delete(uint32_t entityId);

    private:
        template<typename TStruct>
        requires TIsValidEntityStruct<TStruct>
        uint32_t AllocateComposition(bool is_view, size_t newEntryCount)
        {
            using TComposition = TStructToEntityComposition<TStruct>;
            const auto typeIndex = pk_type_index<TComposition>;
            const auto typeKey = (typeIndex & 0x7FFFFFFF) | (is_view << 31ull);
            const auto index = m_compositions.AddKey(typeKey);
            const auto isNew = m_compositions[index].value.componentCount == 0ull;

            if (isNew)
            {
                Memory::Construct(&m_compositions[index].value, 
                    static_cast<uint32_t>(TComposition::Stride),
                    static_cast<uint32_t>(TComposition::Size),
                    component_interfaces<TComposition>.data,
                    0u, 0u, nullptr, nullptr);
            }
            
            if (isNew && is_view)
            {
                UpdateViewIndices(index);
            }

            if (!is_view)
            {
                ReserveEntitities(index, newEntryCount);
            }

            return index;
        }

        template<typename TView>
        requires TIsValidEntityStruct<TView>
        TView BindView(uint32_t compositionIndex, uint32_t arrayOffset)
        {
            auto* composition = &m_compositions[compositionIndex].value;
            TView view{};

            ReflectFields(view, [composition, arrayOffset](auto& field)
            {
                using TField = TRemoveCVRef_T<decltype(field)>;

                if constexpr (TIsPointer<TField>)
                {
                    constexpr const auto uuid = pk_ecs_type_uuid<TRemovePtr_T<TField>>;

                    for (auto i = 0u; i < composition->componentCount; ++i)
                    {
                        if (composition->components[i].typeUUID == uuid)
                        {
                            field = static_cast<TField>(composition->streams[i]) + arrayOffset;
                        }
                    }
                }
            });

            return view;
        }

        Identifier NewEntity(uint32_t entityIndex);
        void DeleteType(uint32_t typeKey);

        uint32_t* GetEntityIdStream(uint32_t entityIndex);
        void ReserveEntitities(uint32_t entityIndex, size_t entryCount);
        void UpdateViewIndices(uint32_t viewIndex);

        HashSet<Identifier, IdentifierHash> m_identifiers;
        HashMap<uint32_t, Composition> m_compositions;
        uint32_t m_idCounter = 0u;
    };
}
