#pragma once
#include "Core/Base/Containers/HashMap.h"
#include "Core/ECS/EntityComposition.h"
#include "Core/ECS/EntityComponentMeta.h"

namespace PK
{
    // @TODO replace with UUID once we have a working hash for templated types.
    struct EntityDatabase
    {
        struct Composition
        {
            const uint32_t componentStride;
            const uint32_t componentCount;
            const EntityComponentMeta* components;
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
            EntityDatabase* entityDb;
            Composition* viewdata;
            TView view;
            uint32_t groupIndex = 0u;
            uint32_t arrayCount = 0u;
            bool is_valid = false;

            constexpr ViewIterator(EntityDatabase* db, Composition* data) noexcept : entityDb(db), viewdata(data), is_valid(Next()) {}
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
                    auto comp = &entityDb->m_compositions[index].value;
                    
                    if (comp->count)
                    {
                        arrayCount = comp->count - 1ull;
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
            EntityDatabase* entityDb;
            Composition* viewdata;

            size_t count() const
            {
                size_t count = 0u;

                for (auto i = 0u; i < viewdata->count; ++i)
                {
                    auto index = static_cast<const uint32_t*>(viewdata->buffer)[i];
                    auto comp = &entityDb->m_compositions[index].value;
                    count += comp->count;
                }

                return count;
            }

            auto begin() const { return ViewIterator<TView>(entityDb, viewdata); }
            auto end() const { return typename ViewIterator<TView>::Sentinel{}; }
        };

        EntityDatabase(size_t compositionCapacity, size_t entityCapacity);
        ~EntityDatabase();

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
                    entity_component_metas<TComposition>.data,
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
            auto* comp = &m_compositions[compositionIndex].value;
            TView view{};

            ReflectFields(view, [comp, arrayOffset](auto& field)
            {
                using TField = TRemoveCVRef_T<decltype(field)>;

                if constexpr (TIsPointer<TField>)
                {
                    constexpr const auto uuid = pk_ecs_type_uuid<TRemovePtr_T<TField>>;

                    for (auto i = 0u; i < comp->componentCount; ++i)
                    {
                        if (comp->components[i].typeUUID == uuid)
                        {
                            field = static_cast<TField>(comp->streams[i]) + arrayOffset;
                            break;
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
